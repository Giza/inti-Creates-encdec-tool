import sys
import os
import zlib
import struct
from enum import Enum, auto
from dataclasses import dataclass
from typing import Optional

# Константы
INTI_BASEKEY = 0xA1B34F58CAD705B2
INTI_CONST1 = 141

class CompMode(Enum):
    COMP_NO = auto()
    COMP_YES = auto()
    COMP_REVERSE = auto()

class EncDecMode(Enum):
    ENCODE = auto()
    DECODE = auto()

@dataclass
class FileType:
    shorthand: str
    password1: str
    password2: Optional[str]
    compressed: CompMode
    headerskip: int
    need_steamid: bool

# Определение поддерживаемых типов файлов
FILE_TYPES = [
    FileType("bft", "bft90210", None, CompMode.COMP_YES, 0, False),
    FileType("obj", "obj90210", None, CompMode.COMP_YES, 0, False),
    FileType("scroll", "scroll90210", None, CompMode.COMP_YES, 0, False),
    FileType("set", "set90210", None, CompMode.COMP_NO, 0, False),
    FileType("snd", "snd90210", None, CompMode.COMP_NO, 0, False),
    FileType("txt", "txt20170401", None, CompMode.COMP_YES, 0, False),
    FileType("txt2", "x4NKvf3U", None, CompMode.COMP_YES, 0, False),
    FileType("json", "json180601", None, CompMode.COMP_YES, 0, False),
    FileType("json2", "xN5sUeRo", None, CompMode.COMP_REVERSE, 0, False),
    FileType("save1", "gYjkJoTX", "zZ2c9VTK", CompMode.COMP_NO, 16, False),
    FileType("save2", "gVTYZ2jk", "JoTXzc9K", CompMode.COMP_NO, 16, False),
    FileType("save3", "gYjkJoTX", "zZ2c9VTK", CompMode.COMP_NO, 16, True),
    FileType("ssbpi", "ssbpi90210", None, CompMode.COMP_NO, 0, False),
]

def inti_keygen(password: str) -> int:
    key = INTI_BASEKEY
    for c in password:
        key += ord(c)
        key *= INTI_CONST1
    return key & 0xFFFFFFFFFFFFFFFF

def inti_encdec(buffer: bytearray, mode: EncDecMode, key: int) -> None:
    blockkey = key
    for i in range(len(buffer)):
        tmp = buffer[i]
        buffer[i] = tmp ^ ((blockkey >> (i & 0x1F)) & 0xFF)
        
        if mode == EncDecMode.ENCODE:
            blockkey += buffer[i]
        else:
            blockkey += tmp
            
        blockkey *= INTI_CONST1
        blockkey &= 0xFFFFFFFFFFFFFFFF

def process_file(in_path: str, out_path: str, file_type: FileType, mode: EncDecMode, steamid: Optional[int] = None) -> None:
    # Генерация ключей
    if file_type.need_steamid and steamid is not None:
        truncated = steamid & 0xFFFFFFFF
        key1 = inti_keygen(f"{file_type.password1}{hex(truncated)[2:]}")
        key2 = inti_keygen(f"{file_type.password2}{hex(truncated)[2:]}") if file_type.password2 else None
    else:
        key1 = inti_keygen(file_type.password1)
        key2 = inti_keygen(file_type.password2) if file_type.password2 else None

    # Чтение входного файла
    with open(in_path, 'rb') as f:
        data = bytearray(f.read())

    if mode == EncDecMode.DECODE:
        if file_type.compressed == CompMode.COMP_NO:
            if file_type.headerskip > 0:
                header = data[:file_type.headerskip]
                body = data[file_type.headerskip:]
                inti_encdec(body, mode, key1)
                if key2:
                    inti_encdec(body, mode, key2)
                data = header + body
            else:
                inti_encdec(data, mode, key1)
                if key2:
                    inti_encdec(data, mode, key2)
                    
        elif file_type.compressed == CompMode.COMP_REVERSE:
            uncompressed_size = struct.unpack('<I', data[:4])[0]
            compressed_data = data[4:]
            decompressed = zlib.decompress(compressed_data)
            data = bytearray(decompressed)
            inti_encdec(data, mode, key1)
            if key2:
                inti_encdec(data, mode, key2)
                
        else:  # COMP_YES
            inti_encdec(data, mode, key1)
            if key2:
                inti_encdec(data, mode, key2)
            uncompressed_size = struct.unpack('<I', data[:4])[0]
            decompressed = zlib.decompress(data[4:])
            data = bytearray(decompressed)

    else:  # ENCODE
        if file_type.compressed == CompMode.COMP_NO:
            if file_type.headerskip > 0:
                header = data[:file_type.headerskip]
                body = data[file_type.headerskip:]
                inti_encdec(body, mode, key1)
                if key2:
                    inti_encdec(body, mode, key2)
                data = header + body
            else:
                inti_encdec(data, mode, key1)
                if key2:
                    inti_encdec(data, mode, key2)
                    
        elif file_type.compressed == CompMode.COMP_REVERSE:
            inti_encdec(data, mode, key1)
            if key2:
                inti_encdec(data, mode, key2)
            compressed = zlib.compress(bytes(data), 9)
            data = bytearray(struct.pack('<I', len(data)) + compressed)
            
        else:  # COMP_YES
            compressed = zlib.compress(bytes(data), 9)
            data = bytearray(struct.pack('<I', len(data)) + compressed)
            inti_encdec(data, mode, key1)
            if key2:
                inti_encdec(data, mode, key2)

    # Запись выходного файла
    with open(out_path, 'wb') as f:
        f.write(data)

def main():
    if len(sys.argv) < 4:
        print("Usage: script.py <d/e> <filetype> [steamid] <infile> <outfile>")
        sys.exit(1)

    command = sys.argv[1].lower()
    filetype = sys.argv[2].lower()
    
    # Поиск типа файла
    file_type = next((ft for ft in FILE_TYPES if ft.shorthand == filetype), None)
    if not file_type:
        print(f"Unknown file type: {filetype}")
        sys.exit(1)

    # Определение режима
    mode = EncDecMode.DECODE if command == 'd' else EncDecMode.ENCODE
    
    # Обработка аргументов
    if file_type.need_steamid:
        if len(sys.argv) < 6:
            print("SteamID required for this file type")
            sys.exit(1)
        steamid = int(sys.argv[3])
        in_path = sys.argv[4]
        out_path = sys.argv[5]
    else:
        steamid = None
        in_path = sys.argv[3]
        out_path = sys.argv[4]

    process_file(in_path, out_path, file_type, mode, steamid)

if __name__ == "__main__":
    main()