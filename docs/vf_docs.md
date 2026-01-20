# CCX File Format Specification (Star Stable / PXEngine)

This document describes the `.CCX` file format used by **Star Stable Entertainment** within the **PXEngine**.  
A `.CCX` file acts as a **file integrity manifest**, allowing the engine to verify that all shipped game files match expected checksums, sizes, and paths.

It contains:
- A global header  
- A list of file entries

---

## 1. File Overview

A `.CCX` file contains:

1. **FileHeader**  
2. **N × FileEntry** records (where `N` is implied by `data_length`)

The file does **not** contain actual game assets — only metadata used to validate them.

---

## 2. FileHeader Structure

The header is always the first 12 bytes of the file.

| Field                | Size | Description                              |
|----------------------|------|------------------------------------------|
| **magic_bytes**      | 4 bytes | File signature identifying a CCX file?   |
| **manifest_version** | 4 bytes (uint32 LE) | Version of the manifest format?          |
| **entry_count**      | 4 bytes (uint32 LE) | Total length of the following data block |


## 3. FileEntry Structure

Each entry describes **one game file** that must be validated.

The structure is variable‑length due to UTF‑8 strings.

### Layout (in order)

| Field | Size | Description |
|-------|------|-------------|
| **name_len** | 4 bytes (uint32 LE) | Length of `file_name` |
| **file_name** | name_len bytes | UTF‑8 filename only (no path) |
| **unknown1** | 8 bytes | Always zero |
| **original_crc** | 4 bytes | CRC32 of the original file |
| **exported_crc** | 4 bytes | CRC32 of the exported/processed file |
| **unknown2** | 4 bytes | Always zero |
| **file_size** | 4 bytes (uint32 LE) | Size of the file in bytes |
| **unknown4** | 8 bytes | Always zero |
| **source_file_number** | 4 bytes (uint32 LE) | Internal file index |
| **unknown5** | 4 bytes | Always zero |
| **path_len** | 4 bytes (uint32 LE) | Length of `file_path` |
| **file_path** | path_len bytes | UTF‑8 full path inside the game data |

## 4. Purpose of the CCX File

The `.CCX` manifest is used by **PXEngine** to:

- Validate that game files exist  
- Verify file integrity using CRC32  
- Ensure correct file sizes  
- Map internal file indices to paths  
- Detect tampering or corruption  

This is part of Star Stable’s **asset validation pipeline**, ensuring the client only loads correct and unmodified game data.

---

## 5. Notes on Unknown Fields

Several fields (`unknown1`, `unknown2`, `unknown4`, `unknown5`) are always zero in observed files.  
These may be:

- Reserved for future use  
- Padding for alignment  
- Legacy fields from older PXEngine versions  

They should be preserved when rewriting CCX files.

---

## 6. Parsing Flow

1. Read the file header `vf_header_t` 
2. Loop over entries `vf_entry_t` until `entry_count` entries are consumed  
3. For each entry:  
   - Read name length  
   - Read name  
   - Read fixed‑size metadata `vf_entry_fixed_t` 
   - Read path length  
   - Read path  

---
