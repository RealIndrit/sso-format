import ctypes
import os

# ------------------------------------------------------------
# Load the DLL
# ------------------------------------------------------------
dll_name = "libsso_formats_core.dll" if os.name == "nt" else "libsso_formats_core.so"
vf = ctypes.CDLL(dll_name)


# ------------------------------------------------------------
# Header struct
# ------------------------------------------------------------
class VFHeader(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("magic_bytes", ctypes.c_uint8 * 4),
        ("manifest_version", ctypes.c_uint32),
        ("entry_count", ctypes.c_uint32),
    ]


# ------------------------------------------------------------
# Entry struct
# ------------------------------------------------------------
class VFEntry(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("file_name", ctypes.c_char_p),
        ("unknown1", ctypes.c_uint8 * 8),
        ("original_crc", ctypes.c_uint8 * 4),
        ("exported_crc", ctypes.c_uint8 * 4),
        ("unknown2", ctypes.c_uint8 * 4),
        ("file_size", ctypes.c_uint32),
        ("unknown4", ctypes.c_uint8 * 8),
        ("source_file_number", ctypes.c_uint32),
        ("unknown5", ctypes.c_uint8 * 4),
        ("file_path", ctypes.c_char_p),
    ]

# ------------------------------------------------------------
# VFFile struct
# ------------------------------------------------------------
class VFFile(ctypes.Structure):
    _fields_ = [
        ("header", VFHeader),
        ("entries", ctypes.POINTER(VFEntry)),
    ]


# ------------------------------------------------------------
# Core API function signatures
# ------------------------------------------------------------
vf.vf_file_read.argtypes = [ctypes.c_char_p]
vf.vf_file_read.restype  = ctypes.POINTER(VFFile)

vf.vf_file_write.argtypes = [ctypes.c_char_p, ctypes.POINTER(VFFile)]
vf.vf_file_write.restype  = ctypes.c_int

vf.vf_file_free.argtypes = [ctypes.POINTER(VFFile)]
vf.vf_file_free.restype  = None

# ------------------------------------------------------------
# Entry lifecycle
# ------------------------------------------------------------
vf.vf_entry_create.argtypes = []
vf.vf_entry_create.restype  = ctypes.POINTER(VFEntry)

vf.vf_entry_free.argtypes = [ctypes.POINTER(VFEntry)]
vf.vf_entry_free.restype  = None


# ------------------------------------------------------------
# File entry management
# ------------------------------------------------------------
vf.vf_file_entry_count.argtypes = [ctypes.POINTER(VFFile)]
vf.vf_file_entry_count.restype  = ctypes.c_uint32

vf.vf_file_get_entry.argtypes = [ctypes.POINTER(VFFile), ctypes.c_uint32]
vf.vf_file_get_entry.restype  = ctypes.POINTER(VFEntry)

vf.vf_file_add_entry.argtypes = [ctypes.POINTER(VFFile), ctypes.POINTER(VFEntry)]
vf.vf_file_add_entry.restype  = ctypes.c_int

vf.vf_file_remove_entry.argtypes = [ctypes.POINTER(VFFile), ctypes.c_uint32]
vf.vf_file_remove_entry.restype  = ctypes.c_int

vf.vf_file_resize.argtypes = [ctypes.POINTER(VFFile), ctypes.c_uint32]
vf.vf_file_resize.restype  = ctypes.c_int


# ------------------------------------------------------------
# String getters/setters
# ------------------------------------------------------------
vf.vf_entry_get_name.argtypes = [ctypes.POINTER(VFEntry)]
vf.vf_entry_get_name.restype  = ctypes.c_char_p

vf.vf_entry_get_path.argtypes = [ctypes.POINTER(VFEntry)]
vf.vf_entry_get_path.restype  = ctypes.c_char_p

vf.vf_entry_set_name.argtypes = [ctypes.POINTER(VFEntry), ctypes.c_char_p]
vf.vf_entry_set_name.restype  = None

vf.vf_entry_set_path.argtypes = [ctypes.POINTER(VFEntry), ctypes.c_char_p]
vf.vf_entry_set_path.restype  = None


# ------------------------------------------------------------
# Integer getters/setters
# ------------------------------------------------------------
vf.vf_entry_get_file_size.argtypes = [ctypes.POINTER(VFEntry)]
vf.vf_entry_get_file_size.restype  = ctypes.c_uint32

vf.vf_entry_set_file_size.argtypes = [ctypes.POINTER(VFEntry), ctypes.c_uint32]
vf.vf_entry_set_file_size.restype  = None

vf.vf_entry_get_source_file_number.argtypes = [ctypes.POINTER(VFEntry)]
vf.vf_entry_get_source_file_number.restype  = ctypes.c_uint32

vf.vf_entry_set_source_file_number.argtypes = [ctypes.POINTER(VFEntry), ctypes.c_uint32]
vf.vf_entry_set_source_file_number.restype  = None


# ------------------------------------------------------------
# Binary block getters/setters
# ------------------------------------------------------------
def _make_block_getter(func, size):
    func.argtypes = [ctypes.POINTER(VFEntry), ctypes.POINTER(ctypes.c_uint8)]
    func.restype = None

    def getter(entry):
        buf = (ctypes.c_uint8 * size)()
        func(entry, buf)
        return bytes(buf)

    return getter


def _make_block_setter(func, size):
    func.argtypes = [ctypes.POINTER(VFEntry), ctypes.POINTER(ctypes.c_uint8)]
    func.restype = ctypes.c_int

    def setter(entry, data: bytes):
        if data is None or len(data) != size:
            raise ValueError(f"Expected {size} bytes")
        arr = (ctypes.c_uint8 * size)(*data)
        error = func(entry, arr)
        if error:
            raise RuntimeError("Setter failed (NULL or invalid input)")

    return setter


get_unknown1  = _make_block_getter(vf.vf_entry_get_unknown1, 8)
set_unknown1  = _make_block_setter(vf.vf_entry_set_unknown1, 8)

get_original_crc = _make_block_getter(vf.vf_entry_get_original_crc, 4)
set_original_crc = _make_block_setter(vf.vf_entry_set_original_crc, 4)

get_exported_crc = _make_block_getter(vf.vf_entry_get_exported_crc, 4)
set_exported_crc = _make_block_setter(vf.vf_entry_set_exported_crc, 4)

get_unknown2  = _make_block_getter(vf.vf_entry_get_unknown2, 4)
set_unknown2  = _make_block_setter(vf.vf_entry_set_unknown2, 4)

get_unknown4  = _make_block_getter(vf.vf_entry_get_unknown4, 8)
set_unknown4  = _make_block_setter(vf.vf_entry_set_unknown4, 8)

get_unknown5  = _make_block_getter(vf.vf_entry_get_unknown5, 4)
set_unknown5  = _make_block_setter(vf.vf_entry_set_unknown5, 4)


# ------------------------------------------------------------
# Utilities: clone
# ------------------------------------------------------------
vf.vf_entry_clone.argtypes = [ctypes.POINTER(VFEntry)]
vf.vf_entry_clone.restype  = ctypes.POINTER(VFEntry)


# ------------------------------------------------------------
# High-level helper API
# ------------------------------------------------------------
def load_vf(path: str) -> ctypes.POINTER(VFFile):
    vf_file = vf.vf_file_read(path.encode("utf-8"))
    if not vf_file:
        raise RuntimeError(f"Failed to load VF file: {path}")
    return vf_file


def save_vf(path: str, vf_file: ctypes.POINTER(VFFile)):
    if vf.vf_file_write(path.encode("utf-8"), vf_file):
        raise RuntimeError(f"Failed to write VF file: {path}")


def free_vf(vf_file: ctypes.POINTER(VFFile)):
    vf.vf_file_free(vf_file)


def get_entry(vf_file: ctypes.POINTER(VFFile), index: int) -> VFEntry:
    ptr = vf.vf_file_get_entry(vf_file, index)
    if not ptr:
        raise IndexError(f"Entry index {index} out of range")
    return ptr.contents


def iter_entries(vf_file: ctypes.POINTER(VFFile)):
    count = vf.vf_file_entry_count(vf_file)
    for i in range(count):
        yield get_entry(vf_file, i)


# ------------------------------------------------------------
# Pythonic convenience wrappers
# ------------------------------------------------------------
def get_entry_name(entry: VFEntry) -> str:
    return vf.vf_entry_get_name(ctypes.byref(entry)).decode("utf-8")


def get_entry_path(entry: VFEntry) -> str:
    return vf.vf_entry_get_path(ctypes.byref(entry)).decode("utf-8")


def set_entry_name(entry: VFEntry, name: str):
    vf.vf_entry_set_name(ctypes.byref(entry), name.encode("utf-8"))


def set_entry_path(entry: VFEntry, path: str):
    vf.vf_entry_set_path(ctypes.byref(entry), path.encode("utf-8"))
