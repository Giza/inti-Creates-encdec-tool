#include <stdint.h>
#include <string.h>
#include "encdec.h"

uint64_t inti_keygen (const char *keystr)
{
  int i, l;
  uint64_t key;

  l = strlen(keystr);
  key = INTI_BASEKEY;

  for (i=0; i<l; i++)
  {
    key += keystr[i];
    key *= 141;
  }

  return key;
}

void inti_encdec (uint8_t *buffer, int mode, int len, uint64_t key)
{
  int i;
  uint64_t blockkey;
  uint8_t tmp;

  blockkey = key;

  for (i=0; i<len; i++)
  {
    tmp = buffer[i];
    buffer[i] = tmp ^ ((blockkey>>(i&0x1F))&0xFF);

    if (mode == MODE_ENC)
      blockkey += buffer[i];
    else
      blockkey += tmp;

    blockkey *= 141;
  }
}