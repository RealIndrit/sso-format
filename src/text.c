#define TEXT_BUILD_DLL
#include "text.h"

#include <stdlib.h>
#include <string.h>

/* ================== INTERNAL HELPERS ================== */

static void shift_bytes(uint8_t *buf, size_t len, int shift) {
    for (size_t i = 0; i < len; i++)
        buf[i] = (uint8_t)((buf[i] + shift) & 0xFF);
}

/* ================== STRUCT I/O ================== */

int text_header_read(FILE *f, text_header_t *h) {
    if (!f || !h) return 1;
    if (io_read_exact(f, h, sizeof(text_header_t)) != 0)
        return 1;
    return 0;
}

int text_header_write(FILE *f, const text_header_t *h) {
    if (!f || !h) return 1;

    if (fwrite(h->unknown,  1, 4, f) != 4) return 1;
    if (fwrite(h->unknown2, 1, 4, f) != 4) return 1;
    if (fwrite(h->unknown3, 1, 4, f) != 4) return 1;

    uint8_t len_buf[4];
    len_buf[0] = (uint8_t)(h->entry_count & 0xFF);
    len_buf[1] = (uint8_t)((h->entry_count >> 8) & 0xFF);
    len_buf[2] = (uint8_t)((h->entry_count >> 16) & 0xFF);
    len_buf[3] = (uint8_t)((h->entry_count >> 24) & 0xFF);

    if (fwrite(len_buf, 1, 4, f) != 4) return 1;
    return 0;
}

int text_entry_read(FILE *f, text_entry_t *e) {
    if (!f || !e) return 1;
    memset(e, 0, sizeof(*e));

    /* key_length */
    if (io_read_exact(f, &e->key_length, 1) != 0) return 1;

    /* unknown (2 bytes) */
    if (io_read_exact(f, e->unknown, 2) != 0) return 1;

    /* key_offset */
    if (io_read_exact(f, &e->key_offset, 1) != 0) return 1;

    /* encoded key */
    if (e->key_length > 0) {
        uint8_t *encoded_key = (uint8_t *)malloc(e->key_length);
        if (!encoded_key) return 1;

        if (io_read_exact(f, encoded_key, e->key_length) != 0) {
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
    } else {
        e->key = NULL;
    }

    /* unknown2 (4 bytes) */
    if (io_read_exact(f, e->unknown2, 4) != 0) return 1;

    /* unknown3 (4 bytes) */
    if (io_read_exact(f, e->unknown3, 4) != 0) return 1;

    /* raw_value_length (4 bytes, little-endian) */
    uint8_t len_buf[4];
    if (io_read_exact(f, len_buf, 4) != 0) return 1;
    uint32_t raw_value_length = (uint32_t)(len_buf[0] |
                                           (len_buf[1] << 8) |
                                           (len_buf[2] << 16) |
                                           (len_buf[3] << 24));
    if (raw_value_length < 2) return 1;
    e->value_length = raw_value_length - 2;

    /* unknown4, unknown5, unknown6 */
    if (io_read_exact(f, &e->unknown4, 1) != 0) return 1;
    if (io_read_exact(f, &e->unknown5, 1) != 0) return 1;
    if (io_read_exact(f, &e->unknown6, 1) != 0) return 1;

    /* value_raw */
    if (e->value_length > 0) {
        uint8_t *value_raw = (uint8_t *)malloc(e->value_length);
        if (!value_raw) return 1;

        if (io_read_exact(f, value_raw, e->value_length) != 0) {
            free(value_raw);
            return 1;
        }

        /* skip trailing 2 bytes */
        uint8_t skip[2];
        if (io_read_exact(f, skip, 2) != 0) {
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

        e->value = malloc(e->value_length);
        if (!e->value) {
            free(value_raw);
            return 1;
        }
        memcpy(e->value, value_raw, e->value_length);
        free(value_raw);
    } else {
        e->value = NULL;
        uint8_t skip[2];
        if (io_read_exact(f, skip, 2) != 0) return 1;
    }

    return 0;
}

/* Mirrors Python TextEntry.write NotImplementedError */
int text_entry_write(FILE *f, const text_entry_t *e) {
    (void)f;
    (void)e;
    return 1; /* not implemented => error */
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

    if (text_header_read(f, &tf->header) != 0) {
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
            if (text_entry_read(f, &tf->entries[i]) != 0) {
                /* cleanup partial */
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
    return 1; /* not implemented => error */
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
