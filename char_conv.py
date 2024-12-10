import sys

def replace_characters(input_file, output_file, replacements):
    """Заменяет символы в файле согласно словарю замен"""
    try:
        with open(input_file, 'r', encoding='utf-8') as f_in:
            text = f_in.read()
            
        # Выполняем замены
        for rus, jap in replacements.items():
            text = text.replace(rus, jap)
            
        with open(output_file, 'w', encoding='utf-8') as f_out:
            f_out.write(text)
            
        print(f"Файл успешно обработан и сохранен как {output_file}")
        
    except FileNotFoundError:
        print(f"Ошибка: Файл {input_file} не найден")
    except Exception as e:
        print(f"Произошла ошибка: {str(e)}")

def main():
    if len(sys.argv) != 3:
        print("Использование: python script.py input.txt output.txt")
        return
        
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    # Словарь замен (можно расширить по необходимости)
    replacements = {
        'А': 'カ',
        'Б': 'ガ',
        'В': 'キ',
        'Г': 'ギ',
        'Д': 'ク',
        'Е': 'グ',
        'Ё': 'ケ',
        'Ж': 'ゲ',
        'З': 'コ',
        'И': 'ゴ',
        'Й': 'サ',
        'К': 'ザ',
        'Л': 'シ',
        'М': 'ジ',
        'Н': 'ス',
        'О': 'ズ',
        'П': 'セ',
        'Р': 'ゼ',
        'С': 'ソ',
        'Т': 'ゾ',
        'У': 'タ',
        'Ф': 'ダ',
        'Х': 'チ',
        'Ц': 'ヂ',
        'Ч': 'ッ',
        'Ш': 'ツ',
        'Щ': 'テ',
        'Ъ': 'デ',
        'Ы': 'ト',
        'Ь': 'ド',
        'Э': 'ナ',
        'Ю': 'ニ',
        'Я': 'ヌ',
        'а': 'ネ',
        'б': 'ノ',
        'в': 'ハ',
        'г': 'バ',
        'д': 'パ',
        'е': 'ヒ',
        'ё': 'ビ',
        'ж': 'ピ',
        'з': 'フ',
        'и': 'ブ',
        'й': 'プ',
        'к': 'ヘ',
        'л': 'ベ',
        'м': 'ペ',
        'н': 'ホ',
        'о': 'ボ',
        'п': 'ポ',
        'р': 'マ',
        'с': 'ミ',
        'т': 'ム',
        'у': 'メ',
        'ф': 'モ',
        'х': 'ャ',
        'ц': 'ヤ',
        'ч': 'ュ',
        'ш': 'ユ',
        'щ': 'ョ',
        'ъ': 'ヨ',
        'ы': 'ラ',
        'ь': 'リ',
        'э': 'ル',
        'ю': 'レ',
        'я': 'ロ'
    }
    
    replace_characters(input_file, output_file, replacements)

if __name__ == '__main__':
    main()
