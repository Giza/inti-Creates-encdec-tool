#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <malloc.h>
#include <stdlib.h>
#include <ctype.h>

#include <zlib.h>

#include "encdec.h"
#include "ttbfile.h"

#define MAXRECORDS 512

typedef uint8_t byte;

static void Error (char *err, ...)
{
  va_list args;

  va_start(args,err);
  vprintf(err,args);
  va_end(args);
  
  printf("\n");
  fflush(stdout);

  exit(-1);
}

static void writestring(FILE *txtfp, byte *str)
{
  int i, len;
  uint8_t chr;

  len = strlen(str);

  for (i=0; i<len; i++)
  {
    chr = str[i];

    if (chr == '\n') // escape newlines
      fprintf(txtfp, "\\n");
    else if (chr == '\r') // carriage returns
      fprintf(txtfp, "\\r");
    else if (chr == '\t') // tabs
      fprintf(txtfp, "\\t");
    else if (chr == '\b') // backspaces
      fprintf(txtfp, "\\b");
    // ...and all chars in C0 control code range, and #127
    else if (chr < 0x20 || chr == 0x7F)
      fprintf(txtfp, "\\x%02X", chr);
    // otherwise let it pass
    else
      fputc(chr,txtfp);
  }
}

static void DumpTTB (FILE *txtfp, byte *ttbdata, int ttblen)
{
  int i, j, numrecords;
  ttbhead_t *ttbhead;
  ttbrec_t *ttbrec;

  ttbhead = (ttbhead_t*)ttbdata;

  if (ttbhead->unknown1 != 0x8 || ttbhead->unknown2 != 0x10)
    Error("DumpTTB: Unexpected TTB header values (0x%x/0x%x should be 0x8/0x10)",
    ttbhead->unknown1, ttbhead->unknown2);
  
  ttbrec = (ttbrec_t*)(ttbdata+sizeof(ttbhead_t));
  numrecords = 0;

  // first, count the records
  while (ttbrec->offset > 32 && ttbrec->offset < ttblen)
  {
    ttbrec++;
    numrecords++;

    if (numrecords > MAXRECORDS)
      Error("too many records");
  }

  // first line in dump file: # of records
  fprintf(txtfp, "%i\n\n", numrecords);

  // now reset the pointer & loop thru header records again
  ttbrec = (ttbrec_t*)(ttbdata+sizeof(ttbhead_t));
  for (i=0; i<numrecords; i++)
  {
    // 1st line: id / unknown data from TTB record
    fprintf(txtfp, "%08X %08X %08X\n", ttbrec->unknown1, ttbrec->unknown2, ttbrec->unknown3);

    // 2nd line: actual string, escaped
    writestring(txtfp, ttbdata+ttbrec->offset);

    // separator
    if (i < numrecords-1)
      fprintf(txtfp,"\n\n");

    ttbrec++;
  }

  return;
}

static void TTB2TXT (char *txtpath, char *ttbpath)
{
  FILE *ttbfp, *txtfp;
  uint32_t clen, ulen;
  uint64_t key;
  byte *cbuf, *ubuf;
  int r;

  ttbfp = fopen(ttbpath, "rb");
  if (!ttbfp)
    Error("TTB2TXT: Failed to open input TTB file '%s'", ttbpath);

  txtfp = fopen(txtpath, "w");
  if (!txtfp)
    Error("TTB2TXT: Failed to open output TXT file '%s'", txtpath);

  fseek(ttbfp, 0, SEEK_END);
  clen = ftell(ttbfp);
  fseek(ttbfp, 0, SEEK_SET);

  if (clen < 32)
    Error("TTB2TXT: Supposed TTB file '%s' is too short", txtpath);

  cbuf = malloc(clen);
  if (!cbuf)
    Error("TTB2TXT: Failed to allocate %i bytes for compressed data buffer", clen);
  fread(cbuf, 1, clen, ttbfp);
  fclose(ttbfp);

  key = inti_keygen("txt20170401");
  inti_encdec(cbuf, MODE_DEC, clen, key);

  // first 4 bytes after descrambling = length of data after zlib decompression
  memcpy(&ulen, cbuf, sizeof(int32_t));

  ubuf = malloc(ulen);
  if (!ubuf)
    Error("TTB2TXT: Failed to allocate %i bytes for decompressed data buffer", ulen);
  
  r = uncompress(ubuf, (uLongf*)&ulen, cbuf+sizeof(int32_t), clen-sizeof(int32_t));
  if (r != Z_OK)
    Error("TTB2TXT: zlib decompression failed, error code %i", r);
  
  free(cbuf);

  DumpTTB(txtfp, ubuf, ulen);

  free(ubuf);
}

#define MAXSTRING 2048
static byte linebuf[MAXSTRING];

typedef struct
{
  uint32_t u1,u2,u3;
  byte *string;
  int length;
  int offset;
}
tmprec_t;

static inline byte convertnybble (byte b)
{
  b = toupper(b);

  if (b >= '0' && b <= '9')
    b -= '0';
  else if (b >= 'A' && b <= 'F')
    b = b-'A'+10;
  else
    Error("convertnybble: bad character encountered");

  return b;
}

int readstring (byte *dst, byte *src)
{
  int si,di;
  int srclen;

  srclen = strlen(src);

  di = si = 0;

  while (si < srclen)
  {
    if (src[si]=='\\')
    {
      if (src[si+1]=='n')
      {
        dst[di++] = '\n';
        si += 2;
      }
      else if (src[si+1]=='r')
      {
        dst[di++] = '\r';
        si += 2;
      }
      else if (src[si+1]=='t')
      {
        dst[di++] = '\t';
        si += 2;
      }
      else if (src[si+1]=='b')
      {
        dst[di++] = '\b';
        si += 2;
      }
      else if (src[si+1]=='x')
      {
        byte b,x,y;

        x = convertnybble(src[si+2]);
        y = convertnybble(src[si+3]);

        b = (x<<4)|y;
        dst[di++] = b;

        si += 4;
      }
      else
        Error("bad escape sequence encountered while parsing string");
    }
    else
      dst[di++] = src[si++];
  }

  dst[di] = 0;

  return di;
}

void TXT2TTB (char *ttbpath, char *txtpath)
{
  FILE *ttbfp, *txtfp;
  int i, j, r, numrecords;
  tmprec_t *tmprecs;
  byte *ttbdata, *zdata;
  uint32_t clen, ulen;
  ttbhead_t ttbhead;
  ttbrec_t ttbrec;
  uint32_t offset;
  uint64_t key;

  ttbfp = fopen(ttbpath, "wb");
  if (!ttbfp)
    Error("TXT2TTB: Failed to open output TTB file '%s'", ttbpath);

  txtfp = fopen(txtpath, "r");
  if (!txtfp)
    Error("TXT2TTB: Failed to open input  TXT file '%s'", txtpath);

  // first line in TXT should be number of records to be included in TTB file
  fgets(linebuf, MAXSTRING, txtfp);

  // windows notepad may put this crap at the beginning of the file when saved as UTF-8...
  if (!memcmp(linebuf,"\xEF\xBB\xBF",3))
    r = sscanf(linebuf+3, "%i\n", &numrecords);
  else
    r = sscanf(linebuf, "%i\n", &numrecords);

  if (r < 1)
    Error("TXT2TTB: Parse error (expected count of records)");

  if (numrecords < 0 || numrecords > MAXRECORDS)
    Error("bad record count on first line (TXT not generated by DumpTTB?)");
  
  // skip separator line
  fgets(linebuf, MAXSTRING, txtfp);

  tmprecs = malloc(sizeof(tmprec_t)*numrecords);
  if (!tmprecs)
    Error("failed to allocate memory for string record table");

  offset = sizeof(ttbhead_t) + numrecords*sizeof(ttbrec_t);

  // then follow all the records, 2 lines per record + separator
  for (i=0; i<numrecords; i++)
  {
    int index;
    uint32_t u1,u2,u3;
    int len;

    fgets(linebuf, MAXSTRING, txtfp);
    
    r = sscanf(linebuf, "%08X %08X %08X\n", &u1, &u2, &u3);
    if (r < 3)
      Error("failed to read string id / unknown data, wrong item count (TXT not generated by DumpTTB?) #2");
    
    fgets(linebuf, MAXSTRING, txtfp);

    tmprecs[i].string = malloc(strlen(linebuf)+1);
    if (!tmprecs[i].string)
      Error("failed to allocate memory for string");

    len = readstring(tmprecs[i].string, linebuf); // handle escapes
    tmprecs[i].offset = offset;
    tmprecs[i].length = len;
    offset += len+1;
    
    tmprecs[i].u1 = u1; tmprecs[i].u2 = u2; tmprecs[i].u3 = u3;

    // skip separator line
    if (i < numrecords-1)
      fgets(linebuf, MAXSTRING, txtfp);
  }

  ttbhead.unknown1 = 0x8;
  ttbhead.unknown2 = 0x10;
  
  ulen = sizeof(ttbhead_t); // main TTB header, always present
  ulen += sizeof(ttbrec_t)*numrecords;

  for (i=0; i<numrecords; i++)
    ulen += tmprecs[i].length+1;

  ttbdata = malloc(ulen);
  if (!ttbdata)
    Error("failed to allocate memory for TTB data buffer");

  memcpy(ttbdata, &ttbhead, sizeof(ttbhead_t));

  for (i=0; i<numrecords; i++)
  {
    ttbrec.offset = tmprecs[i].offset;
    
    ttbrec.unknown1 = tmprecs[i].u1; ttbrec.unknown2 = tmprecs[i].u2; ttbrec.unknown3 = tmprecs[i].u3;

    memcpy(ttbdata+sizeof(ttbhead_t)+i*sizeof(ttbrec_t), &ttbrec, sizeof(ttbrec_t));
    memcpy(ttbdata+tmprecs[i].offset, tmprecs[i].string, tmprecs[i].length);
  }

  clen = compressBound(ulen);
  zdata = malloc(clen+sizeof(uint32_t));
  if (!zdata)
    Error("failed to allocate memory for compressed TTB data buffer");

  memcpy(zdata, &ulen, sizeof(uint32_t));
  r = compress2(zdata+sizeof(uint32_t), (uLongf*)&clen, ttbdata, ulen, 9);
  if (r != Z_OK)
    Error("zlib compression for TTB data failed (code %i)\n", r);

  key = inti_keygen("txt20170401");
  inti_encdec(zdata, MODE_ENC, clen+sizeof(uint32_t), key);

  fwrite(zdata, 1, clen+sizeof(uint32_t), ttbfp);

  fclose(ttbfp);
  fclose(txtfp);

  free(zdata);
  free(ttbdata);

  for(i=0; i<numrecords; i++)
    free(tmprecs[i].string);
  
  free(tmprecs);
}

int main (int argc, char **argv)
{
  char command;

  if (argc < 3)
  {
    printf("usage: textconv <e/d> <infile> <outfile>\n");
    return -1;
  }

  command = toupper(argv[1][0]);

  if (command == 'D')
    TTB2TXT(argv[3],argv[2]);
  else if (command=='E')
    TXT2TTB(argv[3],argv[2]);
  else
  {
    printf("usage: textconv <e/d> <infile> <outfile>\n");
    return -1;
  }

  return 0;
}
