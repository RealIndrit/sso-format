#define VF_BUILD_DLL
#include "vf.h"

#include <stdlib.h>
#include <string.h>

/* ================== INTERNAL HELPERS ================== */

#pragma pack(push, 1)
typedef struct {
    uint8_t  unknown1[8];
    uint8_t  original_crc[4];
    uint8_t  exported_crc[4];
    uint8_t  unknown2[4];
    uint32_t file_size;
    uint8_t  unknown4[8];
    uint32_t source_file_number;
    uint8_t  unknown5[4];
} vf_entry_fixed_t;
#pragma pack(pop)

int vf_header_read(FILE *f, vf_header_t *h) {
    if (!f || !h)
        return 1;
    return io_read_exact(f, h, sizeof(vf_header_t));
}

int vf_header_write(FILE *f, const vf_header_t *h) {
    if (!f || !h)
        return 1;
    return io_write_exact(f, h, sizeof(vf_header_t));
}

int vf_entry_read(FILE *f, vf_entry_t *e) {
    if (!f || !e)
        return 1;

    uint32_t name_len, path_len;

    if (io_read_exact(f, &name_len, 4))
        return 1;

    e->file_name = (char *)malloc(name_len + 1);
    if (!e->file_name)
        return 1;
    if (io_read_exact(f, e->file_name, name_len)) {
        free(e->file_name);
        e->file_name = NULL;
        return 1;
    }
    e->file_name[name_len] = '\0';

    vf_entry_fixed_t blk;
    if (io_read_exact(f, &blk, sizeof(blk)))
        return 1;

    memcpy(e->unknown1,        blk.unknown1,        8);
    memcpy(e->original_crc,    blk.original_crc,    4);
    memcpy(e->exported_crc,    blk.exported_crc,    4);
    memcpy(e->unknown2,        blk.unknown2,        4);
    e->file_size =             blk.file_size;
    memcpy(e->unknown4,        blk.unknown4,        8);
    e->source_file_number =    blk.source_file_number;
    memcpy(e->unknown5,        blk.unknown5,        4);

    if (io_read_exact(f, &path_len, 4))
        return 1;

    e->file_path = (char *)malloc(path_len + 1);
    if (!e->file_path)
        return 1;
    if (io_read_exact(f, e->file_path, path_len)) {
        free(e->file_path);
        e->file_path = NULL;
        return 1;
    }
    e->file_path[path_len] = '\0';

    return 0;
}

int vf_entry_write(FILE *f, const vf_entry_t *e) {
    if (!f || !e)
        return 1;

    uint32_t name_len;
    uint32_t path_len;

    if (!e->file_name || !e->file_path)
        return 1;

    name_len = (uint32_t)strlen(e->file_name);
    path_len = (uint32_t)strlen(e->file_path);

    if (io_write_exact(f, &name_len, 4))
        return 1;
    if (io_write_exact(f, e->file_name, name_len))
        return 1;

    vf_entry_fixed_t blk;

    memcpy(blk.unknown1,        e->unknown1,        8);
    memcpy(blk.original_crc,    e->original_crc,    4);
    memcpy(blk.exported_crc,    e->exported_crc,    4);
    memcpy(blk.unknown2,        e->unknown2,        4);
    blk.file_size =             e->file_size;
    memcpy(blk.unknown4,        e->unknown4,        8);
    blk.source_file_number =    e->source_file_number;
    memcpy(blk.unknown5,        e->unknown5,        4);

    if (io_write_exact(f, &blk, sizeof(blk)))
        return 1;

    if (io_write_exact(f, &path_len, 4))
        return 1;
    if (io_write_exact(f, e->file_path, path_len))
        return 1;

    return 0;
}

/* ================== CORE PUBLIC API ================== */

VF_API vf_file_t *vf_file_read(const char *filename) {
    if (!filename)
        return NULL;

    FILE *f = fopen(filename, "rb");
    if (!f)
        return NULL;

    static char io_buf[1 << 16];
    setvbuf(f, io_buf, _IOFBF, sizeof(io_buf));

    vf_file_t *vf = (vf_file_t *)calloc(1, sizeof(vf_file_t));
    if (!vf) {
        fclose(f);
        return NULL;
    }

    if (vf_header_read(f, &vf->header)) {
        fclose(f);
        free(vf);
        return NULL;
    }

    uint32_t n = vf->header.entry_count;
    if (n == 0) {
        vf->entries = NULL;
        fclose(f);
        return vf;
    }

    vf->entries = (vf_entry_t *)calloc(n, sizeof(vf_entry_t));
    if (!vf->entries) {
        fclose(f);
        free(vf);
        return NULL;
    }

    for (uint32_t i = 0; i < n; ++i) {
        if (vf_entry_read(f, &vf->entries[i])) {
            fclose(f);
            vf_file_free(vf);
            return NULL;
        }
    }

    fclose(f);
    return vf;
}

VF_API int vf_file_write(const char *filename, const vf_file_t *vf) {
    if (!filename || !vf)
        return 1;

    FILE *f = fopen(filename, "wb");
    if (!f)
        return 1;

    static char io_buf[1 << 16];
    setvbuf(f, io_buf, _IOFBF, sizeof(io_buf));

    if (vf_header_write(f, &vf->header)) {
        fclose(f);
        return 1;
    }

    for (uint32_t i = 0; i < vf->header.entry_count; ++i) {
        if (vf_entry_write(f, &vf->entries[i])) {
            fclose(f);
            return 1;
        }
    }

    fclose(f);
    return 0;
}

VF_API void vf_file_free(vf_file_t *vf) {
    if (!vf)
        return;

    if (vf->entries) {
        for (uint32_t i = 0; i < vf->header.entry_count; ++i) {
            free(vf->entries[i].file_name);
            free(vf->entries[i].file_path);
        }
    }

    free(vf->entries);
    free(vf);
}

/* ================== STRING ACCESSORS ================== */

VF_API void vf_entry_set_name(vf_entry_t *e, const char *name) {
    if (!e || !name)
        return;

    size_t len = strlen(name);
    char *buf = (char *)malloc(len + 1);
    if (!buf)
        return;
    memcpy(buf, name, len + 1);
    free(e->file_name);
    e->file_name = buf;
}

VF_API void vf_entry_set_path(vf_entry_t *e, const char *path) {
    if (!e || !path)
        return;

    size_t len = strlen(path);
    char *buf = (char *)malloc(len + 1);
    if (!buf)
        return;
    memcpy(buf, path, len + 1);
    free(e->file_path);
    e->file_path = buf;
}

VF_API const char *vf_entry_get_name(const vf_entry_t *e) {
    if (!e)
        return NULL;
    return e->file_name;
}

VF_API const char *vf_entry_get_path(const vf_entry_t *e) {
    if (!e)
        return NULL;
    return e->file_path;
}

/* ================== ENTRY LIFECYCLE ================== */

VF_API vf_entry_t *vf_entry_create(void) {
    vf_entry_t *e = (vf_entry_t *)calloc(1, sizeof(vf_entry_t));
    if (!e)
        return NULL;

    e->file_name = NULL;
    e->file_path = NULL;
    return e;
}

VF_API void vf_entry_free(vf_entry_t *e) {
    if (!e)
        return;

    free(e->file_name);
    free(e->file_path);
    free(e);
}

/* ================== FILE ENTRY MANAGEMENT ================== */

VF_API uint32_t vf_file_entry_count(const vf_file_t *vf) {
    return vf ? vf->header.entry_count : 0;
}

VF_API vf_entry_t *vf_file_get_entry(vf_file_t *vf, uint32_t index) {
    if (!vf)
        return NULL;
    if (index >= vf->header.entry_count)
        return NULL;
    return &vf->entries[index];
}

VF_API int vf_file_resize(vf_file_t *vf, uint32_t new_count) {
    if (!vf)
        return 1;

    uint32_t old_count = vf->header.entry_count;

    if (new_count == old_count)
        return 0;

    if (new_count == 0) {
        for (uint32_t i = 0; i < old_count; ++i) {
            free(vf->entries[i].file_name);
            free(vf->entries[i].file_path);
        }
        free(vf->entries);
        vf->entries = NULL;
        vf->header.entry_count = 0;
        return 0;
    }

    vf_entry_t *new_entries = (vf_entry_t *)realloc(vf->entries, new_count * sizeof(vf_entry_t));
    if (!new_entries)
        return 1;

    vf->entries = new_entries;

    if (new_count > old_count) {
        memset(&vf->entries[old_count], 0, (new_count - old_count) * sizeof(vf_entry_t));
    } else {
        for (uint32_t i = new_count; i < old_count; ++i) {
            free(vf->entries[i].file_name);
            free(vf->entries[i].file_path);
        }
    }

    vf->header.entry_count = new_count;
    return 0;
}

VF_API int vf_file_add_entry(vf_file_t *vf, const vf_entry_t *src) {
    if (!vf || !src)
        return 1;

    uint32_t old_count = vf->header.entry_count;
    if (vf_file_resize(vf, old_count + 1))
        return 1;

    vf_entry_t *dst = &vf->entries[old_count];
    memset(dst, 0, sizeof(*dst));

    if (src->file_name)
        vf_entry_set_name(dst, src->file_name);
    if (src->file_path)
        vf_entry_set_path(dst, src->file_path);

    memcpy(dst->unknown1,     src->unknown1,     8);
    memcpy(dst->original_crc, src->original_crc, 4);
    memcpy(dst->exported_crc, src->exported_crc, 4);
    memcpy(dst->unknown2,     src->unknown2,     4);
    dst->file_size          = src->file_size;
    memcpy(dst->unknown4,     src->unknown4,     8);
    dst->source_file_number = src->source_file_number;
    memcpy(dst->unknown5,     src->unknown5,     4);

    return 0;
}

VF_API int vf_file_remove_entry(vf_file_t *vf, uint32_t index) {
    if (!vf)
        return 1;

    uint32_t count = vf->header.entry_count;
    if (index >= count)
        return 1;

    free(vf->entries[index].file_name);
    free(vf->entries[index].file_path);

    if (index < count - 1) {
        memmove(&vf->entries[index],
                &vf->entries[index + 1],
                (count - index - 1) * sizeof(vf_entry_t));
    }

    return vf_file_resize(vf, count - 1);
}

/* ================== FIELD GETTERS / SETTERS ================== */

VF_API uint32_t vf_entry_get_file_size(const vf_entry_t *e) {
    if (!e)
        return 1;
    return e->file_size;
}

VF_API void vf_entry_set_file_size(vf_entry_t *e, uint32_t size) {
    if (!e)
        return;
    e->file_size = size;
}

VF_API uint32_t vf_entry_get_source_file_number(const vf_entry_t *e) {
    if (!e)
        return 1;
    return e->source_file_number;
}

VF_API void vf_entry_set_source_file_number(vf_entry_t *e, uint32_t num) {
    if (!e)
        return;
    e->source_file_number = num;
}

VF_API void vf_entry_get_unknown1(const vf_entry_t *e, uint8_t out[8]) {
    if (!e || !out)
        return;
    memcpy(out, e->unknown1, 8);
}

VF_API int vf_entry_set_unknown1(vf_entry_t *e, const uint8_t in[8]) {
    if (!e || !in)
        return 1;
    memcpy(e->unknown1, in, 8);
    return 0;
}

VF_API void vf_entry_get_original_crc(const vf_entry_t *e, uint8_t out[4]) {
    if (!e || !out)
        return;
    memcpy(out, e->original_crc, 4);
}

VF_API int vf_entry_set_original_crc(vf_entry_t *e, const uint8_t in[4]) {
    if (!e || !in)
        return 1;
    memcpy(e->original_crc, in, 4);
    return 0;
}

VF_API void vf_entry_get_exported_crc(const vf_entry_t *e, uint8_t out[4]) {
    if (!e || !out)
        return;
    memcpy(out, e->exported_crc, 4);
}

VF_API int vf_entry_set_exported_crc(vf_entry_t *e, const uint8_t in[4]) {
    if (!e || !in)
        return 1;
    memcpy(e->exported_crc, in, 4);
    return 0;
}

VF_API void vf_entry_get_unknown2(const vf_entry_t *e, uint8_t out[4]) {
    if (!e || !out)
        return;
    memcpy(out, e->unknown2, 4);
}

VF_API int vf_entry_set_unknown2(vf_entry_t *e, const uint8_t in[4]) {
    if (!e || !in)
        return 1;
    memcpy(e->unknown2, in, 4);
    return 0;
}

VF_API void vf_entry_get_unknown4(const vf_entry_t *e, uint8_t out[8]) {
    if (!e || !out)
        return;
    memcpy(out, e->unknown4, 8);
}

VF_API int vf_entry_set_unknown4(vf_entry_t *e, const uint8_t in[8]) {
    if (!e || !in)
        return 1;
    memcpy(e->unknown4, in, 8);
    return 0;
}

VF_API void vf_entry_get_unknown5(const vf_entry_t *e, uint8_t out[4]) {
    if (!e || !out)
        return;
    memcpy(out, e->unknown5, 4);
}

VF_API int vf_entry_set_unknown5(vf_entry_t *e, const uint8_t in[4]) {
    if (!e || !in)
        return 1;
    memcpy(e->unknown5, in, 4);
    return 0;
}

/* ================== UTILITIES ================== */

VF_API vf_entry_t *vf_entry_clone(const vf_entry_t *src) {
    if (!src)
        return NULL;

    vf_entry_t *dst = vf_entry_create();
    if (!dst)
        return NULL;

    if (src->file_name)
        vf_entry_set_name(dst, src->file_name);
    if (src->file_path)
        vf_entry_set_path(dst, src->file_path);

    memcpy(dst->unknown1,     src->unknown1,     8);
    memcpy(dst->original_crc, src->original_crc, 4);
    memcpy(dst->exported_crc, src->exported_crc, 4);
    memcpy(dst->unknown2,     src->unknown2,     4);
    dst->file_size          = src->file_size;
    memcpy(dst->unknown4,     src->unknown4,     8);
    dst->source_file_number = src->source_file_number;
    memcpy(dst->unknown5,     src->unknown5,     4);

    return dst;
}
