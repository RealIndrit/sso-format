#define TEXT_BUILD_DLL
#include "text.h"

#include <stdlib.h>
#include <string.h>

/* ================== INTERNAL HELPERS ================== */

#pragma pack(push, 1)
typedef struct {
    uint8_t key_length;
    uint8_t unknown[2];
    uint8_t key_offset;
} entry_prefix_t;

typedef struct {
    uint8_t unknown2[4];
    uint8_t unknown3[4];
} entry_mid_t;

typedef struct {
    uint32_t raw_value_length;
    uint8_t  unknown4;
    uint8_t  unknown5;
    uint8_t  unknown6;
} entry_value_meta_t;
#pragma pack(pop)

static inline char *dup_string(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *out = (char *)malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, s, len + 1);
    return out;
}

static void shift_bytes(uint8_t *buf, size_t len, int shift) {
    for (size_t i = 0; i < len; i++)
        buf[i] = (uint8_t)((buf[i] + shift) & 0xFF);
}

static char *utf16_to_ascii_hacky(const uint8_t *raw, size_t len_bytes) {
    size_t chars = len_bytes / 2;
    char *out = (char *)malloc(chars + 1);
    if (!out) return NULL;

    for (size_t i = 0; i < chars; i++) {
        uint16_t wc = (uint16_t)(raw[i * 2] | (raw[i * 2 + 1] << 8));
        out[i] = (wc < 0x80) ? (char)wc : '?';
    }

    out[chars] = '\0';
    return out;
}

/* ================== STRUCT I/O ================== */

int text_header_read(FILE *f, text_header_t *h) {
    if (!f || !h) return 1;
    if (io_read_exact(f, h, sizeof(text_header_t)) != 0)
        return 1;
    return 0;
}

int text_header_write(FILE *f, const text_header_t *h) {
    if (!f || !h)
        return 1;

    if (io_write_exact(f, h, sizeof(text_header_t)))
        return 1;

    return 0;
}

int text_entry_read(FILE *f, text_entry_t *e) {
    if (!f || !e) return 1;
    memset(e, 0, sizeof(*e));

    entry_prefix_t prefix;
    if (io_read_exact(f, &prefix, sizeof(prefix)) != 0)
        return 1;

    e->key_length = prefix.key_length;
    memcpy(e->unknown, prefix.unknown, sizeof(e->unknown));
    e->key_offset = prefix.key_offset;

    if (e->key_length > 0) {
        uint8_t *encoded_key = (uint8_t *)malloc(e->key_length);
        if (!encoded_key) return 1;

        if (io_read_exact(f, encoded_key, e->key_length)) {
            free(encoded_key);
            return 1;
        }

        shift_bytes(encoded_key, e->key_length, (int)e->key_offset);

        e->key = (char *)malloc(e->key_length + 1);
        if (!e->key) {
            free(encoded_key);
            return 1;
        }
        memcpy(e->key, encoded_key, e->key_length);
        e->key[e->key_length] = '\0';

        free(encoded_key);
    }

    entry_mid_t mid;
    if (io_read_exact(f, &mid, sizeof(mid)))
        return 1;
    memcpy(e->unknown2, mid.unknown2, sizeof(e->unknown2));
    memcpy(e->unknown3, mid.unknown3, sizeof(e->unknown3));

    entry_value_meta_t meta;
    if (io_read_exact(f, &meta, sizeof(meta)))
        return 1;

    if (meta.raw_value_length < 2)
        return 1;

    e->value_length = meta.raw_value_length - 2;
    e->unknown4 = meta.unknown4;
    e->unknown5 = meta.unknown5;
    e->unknown6 = meta.unknown6;

    if (e->value_length > 0) {
        uint8_t *value_raw = (uint8_t *)malloc(e->value_length);
        if (!value_raw) return 1;

        if (io_read_exact(f, value_raw, e->value_length)) {
            free(value_raw);
            return 1;
        }

        uint8_t skip[2];
        if (io_read_exact(f, skip, sizeof(skip))) {
            free(value_raw);
            return 1;
        }

        if (e->value_length > 1) {
            uint8_t second_byte = value_raw[1];
            e->value_offset = (uint8_t)((256 - second_byte) & 0xFF);
        } else {
            e->value_offset = 0;
        }

        shift_bytes(value_raw, e->value_length, (int)e->value_offset);

        e->value = utf16_to_ascii_hacky(value_raw, e->value_length);
        free(value_raw);

        if (!e->value)
            return 1;

        e->value_length = e->value_length / 2;
    } else {
        e->value = NULL;
        uint8_t skip[2];
        if (io_read_exact(f, skip, sizeof(skip)))
            return 1;
    }

    return 0;
}

int text_entry_write(FILE *f, const text_entry_t *e) {
    (void)f;
    (void)e;
    return 1;
}

/* ================== CORE PUBLIC API ================== */

TEXT_API text_file_t *text_file_read(const char *filename) {
    if (!filename) return NULL;

    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;

    text_file_t *tf = (text_file_t *)calloc(1, sizeof(text_file_t));
    if (!tf) {
        fclose(f);
        return NULL;
    }

    if (text_header_read(f, &tf->header)) {
        fclose(f);
        free(tf);
        return NULL;
    }

    if (tf->header.entry_count > 0) {
        tf->entries = (text_entry_t *)calloc(tf->header.entry_count, sizeof(text_entry_t));
        if (!tf->entries) {
            fclose(f);
            free(tf);
            return NULL;
        }

        for (uint32_t i = 0; i < tf->header.entry_count; ++i) {
            if (text_entry_read(f, &tf->entries[i])) {
                for (uint32_t j = 0; j < i; ++j) {
                    free(tf->entries[j].key);
                    free(tf->entries[j].value);
                }
                free(tf->entries);
                free(tf);
                fclose(f);
                return NULL;
            }
        }
    }

    fclose(f);
    return tf;
}

TEXT_API int text_file_write(const char *filename, const text_file_t *tf) {
    (void)filename;
    (void)tf;
    return 1;
}

TEXT_API void text_file_free(text_file_t *tf) {
    if (!tf) return;
    if (tf->entries) {
        for (uint32_t i = 0; i < tf->header.entry_count; ++i) {
            free(tf->entries[i].key);
            free(tf->entries[i].value);
        }
        free(tf->entries);
    }
    free(tf);
}

/* ================== FILE ENTRY MANAGEMENT ================== */

TEXT_API uint32_t text_file_entry_count(const text_file_t *tf) {
    return tf ? tf->header.entry_count : 0;
}

TEXT_API text_entry_t *text_file_get_entry(text_file_t *tf, uint32_t index) {
    if (!tf || !tf->entries) return NULL;
    if (index >= tf->header.entry_count) return NULL;
    return &tf->entries[index];
}

TEXT_API int text_file_resize(text_file_t *tf, uint32_t new_count) {
    if (!tf) return 1;

    if (new_count == tf->header.entry_count)
        return 0;

    if (new_count == 0) {
        if (tf->entries) {
            for (uint32_t i = 0; i < tf->header.entry_count; ++i) {
                free(tf->entries[i].key);
                free(tf->entries[i].value);
            }
            free(tf->entries);
            tf->entries = NULL;
        }
        tf->header.entry_count = 0;
        return 0;
    }

    text_entry_t *new_entries =
        (text_entry_t *)realloc(tf->entries, new_count * sizeof(text_entry_t));
    if (!new_entries) return 1;

    if (new_count > tf->header.entry_count) {
        memset(&new_entries[tf->header.entry_count], 0,
               (new_count - tf->header.entry_count) * sizeof(text_entry_t));
    }

    tf->entries = new_entries;
    tf->header.entry_count = new_count;
    return 0;
}

TEXT_API int text_file_remove_entry(text_file_t *tf, uint32_t index) {
    if (!tf || !tf->entries) return 1;
    if (index >= tf->header.entry_count) return 1;

    free(tf->entries[index].key);
    free(tf->entries[index].value);

    for (uint32_t i = index; i < tf->header.entry_count - 1; ++i)
        tf->entries[i] = tf->entries[i + 1];

    tf->header.entry_count--;

    if (tf->header.entry_count == 0) {
        free(tf->entries);
        tf->entries = NULL;
        return 0;
    }

    text_entry_t *new_entries =
        (text_entry_t *)realloc(tf->entries, tf->header.entry_count * sizeof(text_entry_t));
    if (new_entries)
        tf->entries = new_entries;

    return 0;
}

TEXT_API int text_file_add_entry(text_file_t *tf, const text_entry_t *src) {
    if (!tf || !src) return 1;

    uint32_t new_count = tf->header.entry_count + 1;

    text_entry_t *new_entries =
        (text_entry_t *)realloc(tf->entries, new_count * sizeof(text_entry_t));
    if (!new_entries) return 1;

    tf->entries = new_entries;

    text_entry_t *dst = &tf->entries[new_count - 1];
    memset(dst, 0, sizeof(*dst));

    if (src->key) {
        dst->key = dup_string(src->key);
        dst->key_length = src->key_length;
    }

    if (src->value) {
        dst->value = dup_string(src->value);
        dst->value_length = src->value_length;
    }

    memcpy(dst->unknown,  src->unknown,  sizeof(dst->unknown));
    memcpy(dst->unknown2, src->unknown2, sizeof(dst->unknown2));
    memcpy(dst->unknown3, src->unknown3, sizeof(dst->unknown3));

    dst->unknown4 = src->unknown4;
    dst->unknown5 = src->unknown5;
    dst->unknown6 = src->unknown6;

    dst->key_offset   = src->key_offset;
    dst->value_offset = src->value_offset;

    tf->header.entry_count = new_count;
    return 0;
}

/* ================== STRING ACCESSORS ================== */

TEXT_API const char *text_entry_get_key(const text_entry_t *e) {
    return e ? e->key : NULL;
}

TEXT_API void text_entry_set_key(text_entry_t *e, const char *key) {
    if (!e) return;

    free(e->key);
    if (!key) {
        e->key = NULL;
        e->key_length = 0;
        return;
    }

    size_t len = strlen(key);
    e->key = (char *)malloc(len + 1);
    if (!e->key) return;

    memcpy(e->key, key, len + 1);
    e->key_length = (uint8_t)len;
}

TEXT_API const char *text_entry_get_value(const text_entry_t *e) {
    return e ? e->value : NULL;
}

TEXT_API void text_entry_set_value(text_entry_t *e, const char *value) {
    if (!e) return;

    free(e->value);
    if (!value) {
        e->value = NULL;
        e->value_length = 0;
        return;
    }

    size_t len = strlen(value);
    e->value = (char *)malloc(len + 1);
    if (!e->value) return;

    memcpy(e->value, value, len + 1);
    e->value_length = (uint32_t)len;
}

/* ================== ENTRY LIFECYCLE ================== */

TEXT_API text_entry_t *text_entry_create(void) {
    text_entry_t *e = (text_entry_t *)calloc(1, sizeof(text_entry_t));
    return e;
}

TEXT_API void text_entry_free(text_entry_t *e) {
    if (!e) return;
    free(e->key);
    free(e->value);
    free(e);
}

/* ================== FIELD GETTERS / SETTERS ================== */

TEXT_API uint8_t text_entry_get_key_offset(const text_entry_t *e) {
    return e ? e->key_offset : 0;
}

TEXT_API void text_entry_set_key_offset(text_entry_t *e, uint8_t off) {
    if (!e) return;
    e->key_offset = off;
}

TEXT_API uint8_t text_entry_get_value_offset(const text_entry_t *e) {
    return e ? e->value_offset : 0;
}

TEXT_API void text_entry_set_value_offset(text_entry_t *e, uint8_t off) {
    if (!e) return;
    e->value_offset = off;
}

TEXT_API void text_entry_get_unknown(const text_entry_t *e, uint8_t out[2]) {
    if (!e || !out) return;
    memcpy(out, e->unknown, 2);
}

TEXT_API int text_entry_set_unknown(text_entry_t *e, const uint8_t in[2]) {
    if (!e || !in) return 1;
    memcpy(e->unknown, in, 2);
    return 0;
}

TEXT_API void text_entry_get_unknown2(const text_entry_t *e, uint8_t out[4]) {
    if (!e || !out) return;
    memcpy(out, e->unknown2, 4);
}

TEXT_API int text_entry_set_unknown2(text_entry_t *e, const uint8_t in[4]) {
    if (!e || !in) return 1;
    memcpy(e->unknown2, in, 4);
    return 0;
}

TEXT_API void text_entry_get_unknown3(const text_entry_t *e, uint8_t out[4]) {
    if (!e || !out) return;
    memcpy(out, e->unknown3, 4);
}

TEXT_API int text_entry_set_unknown3(text_entry_t *e, const uint8_t in[4]) {
    if (!e || !in) return 1;
    memcpy(e->unknown3, in, 4);
    return 0;
}

TEXT_API void text_entry_get_unknown4(const text_entry_t *e, uint8_t *out) {
    if (!e || !out) return;
    *out = e->unknown4;
}

TEXT_API void text_entry_set_unknown4(text_entry_t *e, uint8_t v) {
    if (!e) return;
    e->unknown4 = v;
}

TEXT_API void text_entry_get_unknown5(const text_entry_t *e, uint8_t *out) {
    if (!e || !out) return;
    *out = e->unknown5;
}

TEXT_API void text_entry_set_unknown5(text_entry_t *e, uint8_t v) {
    if (!e) return;
    e->unknown5 = v;
}

TEXT_API void text_entry_get_unknown6(const text_entry_t *e, uint8_t *out) {
    if (!e || !out) return;
    *out = e->unknown6;
}

TEXT_API void text_entry_set_unknown6(text_entry_t *e, uint8_t v) {
    if (!e) return;
    e->unknown6 = v;
}

/* ================== UTILITIES ================== */

TEXT_API text_entry_t *text_entry_clone(const text_entry_t *src) {
    if (!src) return NULL;

    text_entry_t *e = (text_entry_t *)calloc(1, sizeof(text_entry_t));
    if (!e) return NULL;

    if (src->key) {
        e->key = dup_string(src->key);
        e->key_length = src->key_length;
    }

    if (src->value) {
        e->value = dup_string(src->value);
        e->value_length = src->value_length;
    }

    memcpy(e->unknown,  src->unknown,  sizeof(e->unknown));
    memcpy(e->unknown2, src->unknown2, sizeof(e->unknown2));
    memcpy(e->unknown3, src->unknown3, sizeof(e->unknown3));

    e->unknown4 = src->unknown4;
    e->unknown5 = src->unknown5;
    e->unknown6 = src->unknown6;

    e->key_offset   = src->key_offset;
    e->value_offset = src->value_offset;

    return e;
}
