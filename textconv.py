import sys
import zlib
import struct
from typing import List, Tuple
from dataclasses import dataclass

# Добавляем содержимое encdec.py в начало файла
INTI_BASEKEY = 0xA1B34F58CAD705B2

MODE_ENC = 0
MODE_DEC = 1

def inti_keygen(keystr: str) -> int:
    key = INTI_BASEKEY
    
    for c in keystr:
        key += ord(c)
        key *= 141
    
    return key & 0xFFFFFFFFFFFFFFFF

def inti_encdec(buffer: bytearray, mode: int, key: int) -> None:
    blockkey = key
    
    for i in range(len(buffer)):
        tmp = buffer[i]
        buffer[i] = tmp ^ ((blockkey >> (i & 0x1F)) & 0xFF)
        
        if mode == MODE_ENC:
            blockkey += buffer[i]
        else:
            blockkey += tmp
            
        blockkey = (blockkey * 141) & 0xFFFFFFFFFFFFFFFF

@dataclass
class TTBHeader:
    unknown1: int = 0x8  # Всегда 8
    unknown2: int = 0x10 # Всегда 16

@dataclass 
class TTBRecord:
    unknown1: int
    unknown2: int
    unknown3: int
    offset: int
    string: bytes = b''

def error(msg: str, *args):
    print(msg % args)
    sys.exit(-1)

def write_string(f, data: bytes) -> None:
    try:
        # Пробуем декодировать как UTF-8
        text = data.decode('utf-8')
        for char in text:
            if char == '\n':
                f.write('\\n')
            elif char == '\r':
                f.write('\\r')
            elif char == '\t':
                f.write('\\t')
            elif char == '\b':
                f.write('\\b')
            elif ord(char) < 0x20 or ord(char) == 0x7F:
                f.write(f'\\x{ord(char):02X}')
            else:
                f.write(char)
    except UnicodeDecodeError:
        # Если не получилось декодировать как UTF-8, обрабатываем побайтово
        for b in data:
            if b < 0x20 or b == 0x7F:
                f.write(f'\\x{b:02X}')
            else:
                try:
                    f.write(bytes([b]).decode('utf-8'))
                except UnicodeDecodeError:
                    f.write(f'\\x{b:02X}')

def dump_ttb(outfile, ttb_data: bytes):
    header = TTBHeader()
    records: List[TTBRecord] = []
    
    # Читаем заголовок
    pos = 0
    header.unknown1, header.unknown2 = struct.unpack('<II', ttb_data[pos:pos+8])
    pos += 8
    
    if header.unknown1 != 0x8 or header.unknown2 != 0x10:
        error("Unexpected TTB header values (0x%x/0x%x should be 0x8/0x10)",
              header.unknown1, header.unknown2)

    # Читаем записи
    while pos < len(ttb_data):
        rec = TTBRecord(*struct.unpack('<IIII', ttb_data[pos:pos+16]))
        if not (32 < rec.offset < len(ttb_data)):
            break
        records.append(rec)
        pos += 16
        
        if len(records) > 512:
            error("Too many records")

    # Читаем строки
    for rec in records:
        end = ttb_data.find(b'\0', rec.offset)
        rec.string = ttb_data[rec.offset:end]

    # Записываем результат
    with open(outfile, 'w', encoding='utf-8') as f:
        f.write(f'{len(records)}\n\n')
        
        for i, rec in enumerate(records):
            f.write(f'{rec.unknown1:08X} {rec.unknown2:08X} {rec.unknown3:08X}\n')
            write_string(f, rec.string)
            if i < len(records)-1:
                f.write('\n\n')

def read_string(s: str) -> bytes:
    result = bytearray()
    i = 0
    while i < len(s):
        if s[i] == '\\':
            if s[i+1] == 'n':
                result.append(ord('\n'))
                i += 2
            elif s[i+1] == 'r':
                result.append(ord('\r'))
                i += 2
            elif s[i+1] == 't':
                result.append(ord('\t'))
                i += 2
            elif s[i+1] == 'b':
                result.append(ord('\b'))
                i += 2
            elif s[i+1] == 'x':
                hex_str = s[i+2:i+4]
                result.append(int(hex_str, 16))
                i += 4
            else:
                error("Bad escape sequence")
        else:
            # Обрабатываем символ как UTF-8
            char = s[i]
            char_bytes = char.encode('utf-8')
            result.extend(char_bytes)
            i += 1
    return bytes(result)


def ttb2txt(txt_path: str, ttb_path: str):
    # Теперь не нужен import encdec
    with open(ttb_path, 'rb') as f:
        data = bytearray(f.read())
    
    if len(data) < 32:
        error("TTB file too short")
        
    key = inti_keygen("txt20170401")
    inti_encdec(data, MODE_DEC, key)
    
    uncompressed_size = struct.unpack('<I', data[:4])[0]
    decompressed = zlib.decompress(data[4:])
    
    if len(decompressed) != uncompressed_size:
        error("Decompression size mismatch")
        
    dump_ttb(txt_path, decompressed)

def txt2ttb(ttb_path: str, txt_path: str):    
    records: List[TTBRecord] = []
    
    with open(txt_path, 'r', encoding='utf-8') as f:
        # Читаем количество записей
        line = f.readline().strip()
        if line.startswith('\ufeff'):  # BOM
            line = line[1:]
        num_records = int(line)
        
        if not 0 <= num_records <= 512:
            error("Bad record count")
            
        f.readline()  # Пропускаем пустую строку
        
        # Читаем записи
        for _ in range(num_records):
            line = f.readline().strip()
            u1, u2, u3 = map(lambda x: int(x, 16), line.split())
            
            string = read_string(f.readline().strip())
            records.append(TTBRecord(u1, u2, u3, 0, string))
            
            if _ < num_records - 1:
                f.readline()  # Пропускаем разделитель

    # Формируем TTB данные
    ttb_data = bytearray()
    
    # Заголовок
    ttb_data.extend(struct.pack('<II', 0x8, 0x10))
    
    # Вычисляем смещения
    offset = 8 + 16 * len(records)  # Заголовок + записи
    for rec in records:
        rec.offset = offset
        offset += len(rec.string) + 1  # +1 для нулевого байта
        
    # Записываем записи
    for rec in records:
        ttb_data.extend(struct.pack('<IIII', 
            rec.unknown1, rec.unknown2, rec.unknown3, rec.offset))
    
    # Записываем строки
    for rec in records:
        ttb_data.extend(rec.string)
        ttb_data.append(0)  # Завершающий нуль
        
    # Сжимаем
    compressed = zlib.compress(ttb_data, level=9)
    
    # Формируем финальные данные
    final_data = bytearray()
    final_data.extend(struct.pack('<I', len(ttb_data)))
    final_data.extend(compressed)
    
    # Шифруем
    key = inti_keygen("txt20170401")
    inti_encdec(final_data, MODE_ENC, key)
    
    # Записываем результат
    with open(ttb_path, 'wb') as f:
        f.write(final_data)

def main():
    if len(sys.argv) != 4:
        print("Usage: textconv.py <e/d> <infile> <outfile>")
        return 1
        
    command = sys.argv[1].upper()
    
    if command == 'D':
        ttb2txt(sys.argv[3], sys.argv[2])
    elif command == 'E':
        txt2ttb(sys.argv[3], sys.argv[2])
    else:
        print("Usage: textconv.py <e/d> <infile> <outfile>")
        return 1
        
    return 0

if __name__ == '__main__':
    sys.exit(main())