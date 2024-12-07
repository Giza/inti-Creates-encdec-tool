import sys
from PIL import Image
import numpy as np

def raw_to_png(raw_path, width, height, output_path):
    """Конвертирует RAW RGBA8888 в PNG"""
    with open(raw_path, 'rb') as f:
        # Читаем первые 40 байт
        header = f.read(40)
        # Читаем следующие 4 байта как смещение
        offset_bytes = f.read(4)
        offset = int.from_bytes(offset_bytes, byteorder='little')
        print(f"Смещение: {offset}")
        
        # Переходим на позицию, указанную в смещении
        f.seek(offset)
        
        # Читаем данные изображения
        needed_size = width * height * 4  # 4 байта на пиксель (RGBA)
        raw_data = f.read(needed_size)
    
    # Преобразуем байты в массив numpy
    rgba_array = np.frombuffer(raw_data, dtype=np.uint8).reshape(height, width, 4)
    
    # Создаем изображение из массива
    image = Image.fromarray(rgba_array, 'RGBA')
    
    # Сохраняем как PNG
    image.save(output_path, 'PNG')

def png_to_raw(png_path, output_path):
    """Конвертирует PNG в RAW RGBA8888"""
    # Открываем PNG файл
    image = Image.open(png_path)
    
    # Конвертируем в RGBA если нужно
    if image.mode != 'RGBA':
        image = image.convert('RGBA')
    
    # Получаем массив пикселей
    rgba_array = np.array(image)
    
    # Записываем байты в файл
    with open(output_path, 'wb') as f:
        f.write(rgba_array.tobytes())

def main():
    if len(sys.argv) < 2:
        print("Использование:")
        print("Для конвертации RAW в PNG:")
        print("python script.py raw2png input.raw width height output.png")
        print("Для конвертации PNG в RAW:")
        print("python script.py png2raw input.png output.raw")
        return

    command = sys.argv[1]
    
    if command == 'raw2png':
        if len(sys.argv) != 6:
            print("Неверное количество аргументов для raw2png")
            return
        raw_path = sys.argv[2]
        width = int(sys.argv[3])
        height = int(sys.argv[4])
        output_path = sys.argv[5]
        raw_to_png(raw_path, width, height, output_path)
        
    elif command == 'png2raw':
        if len(sys.argv) != 4:
            print("Неверное количество аргументов для png2raw")
            return
        png_path = sys.argv[2]
        output_path = sys.argv[3]
        png_to_raw(png_path, output_path)
    
    else:
        print("Неизвестная команда")

if __name__ == '__main__':
    main()
