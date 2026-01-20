#ifndef TEXT_H
#define TEXT_H

#include <stdint.h>
#include <stdio.h>
#include "io.h"

#ifdef _WIN32
    #ifdef TEXT_BUILD_DLL
        #define TEXT_API __declspec(dllexport)
    #else
        #define TEXT_API __declspec(dllimport)
    #endif
#else
    #define TEXT_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
typedef struct {
    uint8_t  unknown[4];
    uint8_t  unknown2[4];
    uint8_t  unknown3[4];
    uint32_t entry_count;
} text_header_t;
#pragma pack(pop)

typedef struct {
    uint8_t  key_length;
    uint8_t  unknown[2];
    uint8_t  key_offset;
    char    *key;

    uint8_t  unknown2[4];
    uint8_t  unknown3[4];

    uint32_t value_length;
    uint8_t  unknown4;
    uint8_t  unknown5;
    uint8_t  unknown6;

    uint8_t  value_offset;
    char    *value;
} text_entry_t;

typedef struct {
    text_header_t header;
    text_entry_t *entries;
} text_file_t;

/* ================== INTERNAL STRUCT I/O ================== */

int         text_header_read(FILE *f, text_header_t *h);
int         text_header_write(FILE *f, const text_header_t *h);

int         text_entry_read(FILE *f, text_entry_t *e);
int         text_entry_write(FILE *f, const text_entry_t *e);

/* ================== CORE PUBLIC API ================== */

TEXT_API text_file_t *text_file_read(const char *filename);
TEXT_API int          text_file_write(const char *filename, const text_file_t *tf);
TEXT_API void         text_file_free(text_file_t *tf);

/* ================== FILE ENTRY MANAGEMENT ================== */

TEXT_API uint32_t     text_file_entry_count(const text_file_t *tf);
TEXT_API text_entry_t *text_file_get_entry(text_file_t *tf, uint32_t index);

#ifdef __cplusplus
}
#endif

#endif /* TEXT_H */
