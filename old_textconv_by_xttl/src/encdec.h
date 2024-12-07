#ifndef __ENCDEC_H__
#define __ENCDEC_H__

#include <stdint.h>

#define INTI_BASEKEY 0xA1B34F58CAD705B2ULL

enum {
  MODE_ENC,
  MODE_DEC
};

extern uint64_t inti_keygen (const char *keystr);
extern void inti_encdec (uint8_t *buffer, int mode, int len, uint64_t key);

#endif // __ENCDEC_H__