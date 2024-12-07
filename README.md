# Inti File Encoder/Decoder

Python port of the Inti Creates file encoder/decoder tools. This tools allows you to decrypt and encrypt various file formats used in Inti Creates games.

## About
This is a Python conversion of the original C program created for handling Inti Creates encrypted files. The original version can be found in the `old` directory and was sourced from [ZenHax Forum](https://zenhax.com/viewtopic.php@t=15220.html).

## Supported File Types
- `txt`, `txt2` - Text resource files (*.ttb, *.tb2)
- `bft` - BMPFont files (*.bfb)
- `obj` - Object files (*.osb)
- `scroll` - Stage background files (*.scb)
- `set` - Stage setup files (*.stb)
- `snd` - Sound index files (*.bisar)
- `json`, `json2` - Configuration files
- `save1`, `save2`, `save3` - Save data files (requires SteamID for save3)

## Usage

### Basic Usage

Decoding
```
python inti_encdec.py d <filetype> <input_file> <output_file>
```

Encoding
```
python inti_encdec.py e <filetype> <input_file> <output_file>
```

### Examples

Decode a text resource file
```
python inti_encdec.py d txt encrypted.ttb decrypted.txt
```
Encode a text resource file
```
python inti_encdec.py e txt decrypted.txt encrypted.ttb
```
Decode a save file that requires SteamID
```
python inti_encdec.py d save3 12345678 encrypted.sav decrypted.txt
```

# INTI TextConv

Text file converter for INTI CREATES games. Python version.

## Usage
```
python textconv.py <mode> <input_file> <output_file>
```
where:
- `mode`: 
  - `d` - unpack TTB to text file
  - `e` - pack text file to TTB
- `input_file` - path to source file
- `output_file` - path to destination file

### Examples:

Unpacking TTB file:
```
python textconv.py d input.ttb output.txt
```
Packing text file:
```
python textconv.py e input.txt output.ttb
```
## Project Structure

- `textconv.py` - main program file
- `old_textconv_by_xttl/` - original C version by xttl


## Changes from Original Version
- Converted from C to Python
- Simplified memory handling using Python's built-in features
- Uses Python's zlib module instead of direct zlib library calls
- Maintains all original functionality and file type support
- Original C version preserved in `old` directory for reference

## Requirements
- Python 3.7+

## Credits
Original C version by xttl author from ZenHax community
