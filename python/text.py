import ctypes
import os

# ------------------------------------------------------------
# Load the DLL
# ------------------------------------------------------------
dll_name = "libsso_formats_core.dll" if os.name == "nt" else "libsso_formats_core.so"
text = ctypes.CDLL(dll_name)


# ------------------------------------------------------------
# Header struct
# ------------------------------------------------------------
class TextHeader(ctypes.Structure):
    _fields_ = [
        ("unknown", ctypes.c_uint8 * 4),
        ("unknown2", ctypes.c_uint8 * 4),
        ("unknown3", ctypes.c_uint8 * 4),
        ("entry_count", ctypes.c_uint32),
    ]


# ------------------------------------------------------------
# Entry struct
# ------------------------------------------------------------
class TextEntry(ctypes.Structure):
    _fields_ = [
        ("unknown", ctypes.c_uint8 * 2),
        ("key_offset", ctypes.c_uint8),
        ("key", ctypes.c_char_p),
        ("unknown2", ctypes.c_uint8 * 4),
        ("unknown3", ctypes.c_uint8 * 4),
        ("value_length", ctypes.c_uint32),
        ("unknown4", ctypes.c_uint8),
        ("unknown5", ctypes.c_uint8),
        ("unknown6", ctypes.c_uint8),
        ("value_offset", ctypes.c_uint8),
        ("value", ctypes.c_void_p), #Because cpython
    ]


# ------------------------------------------------------------
# File struct
# ------------------------------------------------------------
class TextFile(ctypes.Structure):
    _fields_ = [
        ("header", TextHeader),
        ("entries", ctypes.POINTER(TextEntry)),
    ]


# ------------------------------------------------------------
# Core API signatures
# ------------------------------------------------------------
text.text_file_read.argtypes = [ctypes.c_char_p]
text.text_file_read.restype  = ctypes.POINTER(TextFile)

text.text_file_write.argtypes = [ctypes.c_char_p, ctypes.POINTER(TextFile)]
text.text_file_write.restype  = ctypes.c_int

text.text_file_free.argtypes = [ctypes.POINTER(TextFile)]
text.text_file_free.restype  = None


# ------------------------------------------------------------
# Entry lifecycle
# ------------------------------------------------------------
text.text_entry_create.argtypes = []
text.text_entry_create.restype  = ctypes.POINTER(TextEntry)

text.text_entry_free.argtypes = [ctypes.POINTER(TextEntry)]
text.text_entry_free.restype  = None


# ------------------------------------------------------------
# File entry management
# ------------------------------------------------------------
text.text_file_entry_count.argtypes = [ctypes.POINTER(TextFile)]
text.text_file_entry_count.restype  = ctypes.c_uint32

text.text_file_get_entry.argtypes = [ctypes.POINTER(TextFile), ctypes.c_uint32]
text.text_file_get_entry.restype  = ctypes.POINTER(TextEntry)

text.text_file_add_entry.argtypes = [ctypes.POINTER(TextFile), ctypes.POINTER(TextEntry)]
text.text_file_add_entry.restype  = ctypes.c_int

text.text_file_remove_entry.argtypes = [ctypes.POINTER(TextFile), ctypes.c_uint32]
text.text_file_remove_entry.restype  = ctypes.c_int

text.text_file_resize.argtypes = [ctypes.POINTER(TextFile), ctypes.c_uint32]
text.text_file_resize.restype  = ctypes.c_int


# ------------------------------------------------------------
# String getters/setters
# ------------------------------------------------------------
text.text_entry_get_key.argtypes = [ctypes.POINTER(TextEntry)]
text.text_entry_get_key.restype  = ctypes.c_char_p

text.text_entry_set_key.argtypes = [ctypes.POINTER(TextEntry), ctypes.c_char_p]
text.text_entry_set_key.restype  = None

text.text_entry_get_value.argtypes = [ctypes.POINTER(TextEntry)]
text.text_entry_get_value.restype  = ctypes.c_void_p   # raw UTF‑16 bytes, ctypes cant handle 00 bytes

text.text_entry_set_value.argtypes = [ctypes.POINTER(TextEntry), ctypes.c_char_p]
text.text_entry_set_value.restype  = None


# ------------------------------------------------------------
# Unknown field getters/setters
# ------------------------------------------------------------
def _make_block_getter(func, size):
    func.argtypes = [ctypes.POINTER(TextEntry), ctypes.POINTER(ctypes.c_uint8)]
    func.restype = None

    def getter(entry):
        buf = (ctypes.c_uint8 * size)()
        func(entry, buf)
        return bytes(buf)

    return getter


def _make_block_setter(func, size):
    func.argtypes = [ctypes.POINTER(TextEntry), ctypes.POINTER(ctypes.c_uint8)]
    func.restype = ctypes.c_int

    def setter(entry, data: bytes):
        if data is None or len(data) != size:
            raise ValueError(f"Expected {size} bytes")
        arr = (ctypes.c_uint8 * size)(*data)
        err = func(entry, arr)
        if err:
            raise RuntimeError("Setter failed")

    return setter


# ------------------------------------------------------------
# High-level helper API
# ------------------------------------------------------------
def load_text(path: str) -> ctypes.POINTER(TextFile):
    tf = text.text_file_read(path.encode("utf-8"))
    if not tf:
        raise RuntimeError(f"Failed to load text file: {path}")
    return tf


def save_text(path: str, tf: ctypes.POINTER(TextFile)):
    if text.text_file_write(path.encode("utf-8"), tf):
        raise RuntimeError(f"Failed to write text file: {path}")


def free_text(tf: ctypes.POINTER(TextFile)):
    text.text_file_free(tf)


def get_entry(tf: ctypes.POINTER(TextFile), index: int) -> TextEntry:
    ptr = text.text_file_get_entry(tf, index)
    if not ptr:
        raise IndexError(f"Entry index {index} out of range")
    return ptr.contents


def iter_entries(tf: ctypes.POINTER(TextFile)):
    count = text.text_file_entry_count(tf)
    for i in range(count):
        yield get_entry(tf, i)


# ------------------------------------------------------------
# Pythonic convenience wrappers
# ------------------------------------------------------------
def get_key(entry: TextEntry) -> str:
    raw = text.text_entry_get_key(ctypes.byref(entry))
    return raw.decode("utf-8") if raw else ""

def set_key(entry: TextEntry, key: str):
    text.text_entry_set_key(ctypes.byref(entry), key.encode("utf-8"))

def get_value(entry: TextEntry) -> str:
    ptr = text.text_entry_get_value(ctypes.byref(entry))
    if not ptr:
        return ""

    length = entry.value_length
    raw = ctypes.string_at(ptr, length)
    return raw.decode("utf-16-le")

def set_value(entry: TextEntry, value_utf8: str):
    """Encode UTF‑8 → UTF‑16 and pass to C."""
    utf16 = value_utf8.encode("utf-16-le")
    text.text_entry_set_value(ctypes.byref(entry), utf16)
