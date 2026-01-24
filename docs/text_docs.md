# TEXT File Format Specification (Star Stable / PXEngine)

This document describes the `.text` file format used by **Star Stable Entertainment** within the **PXEngine**.  
A `.text` file acts as a **localization string table**, mapping string keys to (typically UTF‑16LE) text values used throughout the game.

It contains:
- A global header
- A list of text entries (key → value pairs)

------------------------------------------------------------
## Endianness Requirement
------------------------------------------------------------

The PXEngine framework and all associated tooling operate exclusively on little‑endian systems.  
All multi‑byte integer fields in `.text` files are stored and interpreted in **little‑endian** format.  
Big‑endian platforms are not supported.

------------------------------------------------------------
## 1. File Overview
------------------------------------------------------------

A `.text` file contains:

1. **TextHeader**
2. **N × TextEntry** records (where `N` is implied by `data_length`)

The file does **not** contain any executable logic — only string metadata and text content.

------------------------------------------------------------
## 2. TextHeader Structure
------------------------------------------------------------

The header is always the first 16 bytes of the file.

| Field         | Size                 | Description                          |
|---------------|----------------------|--------------------------------------|
| unknown       | 4 bytes              | Reserved / unknown                   |
| unknown2      | 4 bytes              | Reserved / unknown                   |
| unknown3      | 4 bytes              | Reserved / unknown                   |
| entry_count   | 4 bytes (uint32 LE)  | Number of text entries in the file   |

`entry_count` defines how many `TextEntry` records follow.

------------------------------------------------------------
## 3. TextEntry Structure
------------------------------------------------------------

Each entry describes **one localized string**, identified by a key.

The structure is variable‑length due to the key and value strings.

### Layout (in order, on disk)

| Field            | Size                     | Description |
|------------------|--------------------------|-------------|
| key_length       | 1 byte (uint8)           | Length of `key` in bytes (UTF‑8) |
| unknown          | 2 bytes                  | Reserved / unknown               |
| key_offset       | 1 byte (uint8)           | Key encoding/offset parameter    |
| key              | key_length bytes         | UTF‑8 key string                 |
| unknown2         | 4 bytes                  | Reserved / unknown               |
| unknown3         | 4 bytes                  | Reserved / unknown               |
| value_length     | 4 bytes (uint32 LE)      | Length of `value` in bytes (UTF‑16LE), including terminator |
| unknown4         | 1 byte                   | Reserved / unknown               |
| unknown5         | 1 byte                   | Reserved / unknown               |
| unknown6         | 1 byte                   | Reserved / unknown               |
| value_offset     | 1 byte (uint8)           | Value encoding/offset parameter  |
| value            | value_length bytes       | UTF‑16LE text (obfuscated), including `00 00` terminator |

### Key

- Stored as **UTF‑8**.
- Length is given by `key_length`.
- `key_offset` is used to reverse the Caesar‑style byte shift applied to the key string.

### Value

- Stored as **UTF‑16LE**, with a trailing `00 00` null terminator.
- `value_length` is the total number of bytes, including the terminator.
- The value bytes (except the last two) are **obfuscated** using `value_offset`.
- The **last two bytes** are stored as plain `00 00` to preserve the UTF‑16 terminator.

------------------------------------------------------------
## 4. Purpose of the TEXT File
------------------------------------------------------------

The `.text` manifest is used by **PXEngine** to:

- Map internal string keys to localized text values
- Load language‑specific resources at runtime
- Keep string data separate from code and assets
- Support patching and updating text without touching binaries

It is part of the game’s **localization pipeline**, ensuring consistent and efficient access to UI text, dialog, and other in‑game strings.

------------------------------------------------------------
## 5. Notes on Unknown Fields
------------------------------------------------------------

Several fields (`unknown`, `unknown2`, `unknown3`, `unknown4`, `unknown5`, `unknown6`) are not yet fully understood.  
In observed files they appear to be either constant or unused.

They may be:

- Reserved for future metadata
- Alignment/padding helpers
- Legacy fields from older formats or tools

------------------------------------------------------------
## 6. Parsing Flow
------------------------------------------------------------

A typical parser for `.text` files follows this flow:

1. Read the file header `text_header_t`
2. Loop over entries `text_entry_t` until `entry_count` entries are consumed