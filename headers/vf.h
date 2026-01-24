#ifndef VF_H
#define VF_H

#include <stdint.h>
#include <stdio.h>
#include "io.h"

#ifdef _WIN32
    #ifdef VF_BUILD_DLL
        #define VF_API __declspec(dllexport)
    #else
        #define VF_API __declspec(dllimport)
    #endif
#else
    #define VF_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)
typedef struct {
    uint8_t  magic_bytes[4];
    uint32_t manifest_version;
    uint32_t entry_count;
} vf_header_t;
#pragma pack(pop)

typedef struct {
    char    *file_name;
    uint8_t  unknown1[8];
    uint8_t  original_crc[4];
    uint8_t  exported_crc[4];
    uint8_t  unknown2[4];
    uint32_t file_size;
    uint8_t  unknown4[8];
    uint32_t source_file_number;
    uint8_t  unknown5[4];
    char    *file_path;
} vf_entry_t;

typedef struct {
    vf_header_t header;
    vf_entry_t *entries;
} vf_file_t;


int        vf_header_read(FILE *f, vf_header_t *h);
int        vf_header_write(FILE *f, const vf_header_t *h);

int        vf_entry_read(FILE *f, vf_entry_t *e);
int        vf_entry_write(FILE *f, const vf_entry_t *e);

/* ================== CORE PUBLIC API ================== */

VF_API vf_file_t *vf_file_read(const char *filename);
VF_API int        vf_file_write(const char *filename, const vf_file_t *vf);
VF_API void       vf_file_free(vf_file_t *vf);

/* ================== STRING ACCESSORS ================== */

VF_API void        vf_entry_set_name(vf_entry_t *e, const char *name);
VF_API void        vf_entry_set_path(vf_entry_t *e, const char *path);

VF_API const char *vf_entry_get_name(const vf_entry_t *e);
VF_API const char *vf_entry_get_path(const vf_entry_t *e);

/* ================== ENTRY LIFECYCLE ================== */

VF_API vf_entry_t *vf_entry_create(void);
VF_API void        vf_entry_free(vf_entry_t *e);

/* ================== FILE ENTRY MANAGEMENT ================== */

VF_API uint32_t    vf_file_entry_count(const vf_file_t *vf);
VF_API vf_entry_t *vf_file_get_entry(vf_file_t *vf, uint32_t index);

VF_API int         vf_file_add_entry(vf_file_t *vf, const vf_entry_t *src);
VF_API int         vf_file_remove_entry(vf_file_t *vf, uint32_t index);
VF_API int         vf_file_resize(vf_file_t *vf, uint32_t new_count);

/* ================== FIELD GETTERS / SETTERS ================== */

VF_API uint32_t    vf_entry_get_file_size(const vf_entry_t *e);
VF_API void        vf_entry_set_file_size(vf_entry_t *e, uint32_t size);

VF_API uint32_t    vf_entry_get_source_file_number(const vf_entry_t *e);
VF_API void        vf_entry_set_source_file_number(vf_entry_t *e, uint32_t num);

VF_API void        vf_entry_get_unknown1(const vf_entry_t *e, uint8_t out[8]);
VF_API int         vf_entry_set_unknown1(vf_entry_t *e, const uint8_t in[8]);

VF_API void        vf_entry_get_original_crc(const vf_entry_t *e, uint8_t out[4]);
VF_API int         vf_entry_set_original_crc(vf_entry_t *e, const uint8_t in[4]);

VF_API void        vf_entry_get_exported_crc(const vf_entry_t *e, uint8_t out[4]);
VF_API int         vf_entry_set_exported_crc(vf_entry_t *e, const uint8_t in[4]);

VF_API void        vf_entry_get_unknown2(const vf_entry_t *e, uint8_t out[4]);
VF_API int         vf_entry_set_unknown2(vf_entry_t *e, const uint8_t in[4]);

VF_API void        vf_entry_get_unknown4(const vf_entry_t *e, uint8_t out[8]);
VF_API int         vf_entry_set_unknown4(vf_entry_t *e, const uint8_t in[8]);

VF_API void        vf_entry_get_unknown5(const vf_entry_t *e, uint8_t out[4]);
VF_API int         vf_entry_set_unknown5(vf_entry_t *e, const uint8_t in[4]);

/* ================== UTILITIES ================== */

VF_API vf_entry_t *vf_entry_clone(const vf_entry_t *src);

#ifdef __cplusplus
}
#endif

#endif /* VF_H */
