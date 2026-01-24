# SSO Formats Core
**Open‑source documentation and tooling for Star Stable Online’s proprietary file formats.**  
Fast. Minimal. Developer‑friendly.

> **PS:** This project is developed in my free time — usually when I’m bored enough to open Ghidra and reverse‑engineer Star Stable Online’s internal logic, or when I need more tool coverage for my own private modding utilities for the game.  
> If something breaks, please open an issue… or even better, submit a PR!

---

## Project Overview

**SSO Formats Core** is an open‑source initiative to fully document, parse, and manipulate the proprietary file formats used by **Star Stable Online** and the underlying **PXEngine**.

The goals are simple:

- Make these formats **easy to understand**
- Provide **clean, well‑documented reference implementations**
- Keep everything **fast enough for real‑time tooling**
- Ensure the project remains **fully open‑source** and accessible

Whether you're building modding tools, research utilities, or simply exploring how the game stores its data, this project aims to be the definitive reference.

---

## Supported Formats

### `.text` — Localization String Tables
- UTF‑8 keys
- UTF‑16LE values
- Caesar‑style obfuscation
- Full specification included
- C and Python reference implementations

### `.ccx` — File Integrity Manifests
- CRC32 validation
- File size + path metadata
- Used by PXEngine to verify shipped assets
- Fully documented structure

More formats will be added as they are researched and verified.

---

## Features

- **Clean C API** for maximum performance
- **Python bindings** for rapid tooling
- **Zero external dependencies**
- **Little‑endian optimized** (matching PXEngine’s design)
- **Memory‑safe wrappers** for easy integration
- **Readable specifications** for each file type
- **Fast parsing** (targeting ~100ns to ~300ns per entry in C depending on hardware)

---

## Documentation

Each file format includes:

- A full technical specification
- Struct layouts
- Parsing flow
- Notes on unknown or reserved fields
- Reference code for reading/writing
- Examples and test files (when available)

All documentation lives in the `docs/` directory.
