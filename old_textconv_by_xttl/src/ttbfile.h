#ifndef __TTBFILE_H__
#define __TTBFILE_H__

//#define TTBIDSZ 12

#ifdef _MSC_VER
 #pragma pack(push,1)
 #define GCCPACK
#else
 #define GCCPACK __attribute__((packed))
#endif

typedef struct GCCPACK ttbhead_s
{
  uint32_t unknown1 GCCPACK; // maybe version? (seems to be always 0x00000008)
  uint32_t unknown2 GCCPACK; // maybe record length? (seems to be always 0x00000010, which matches sizeof next struct)
}
ttbhead_t GCCPACK;

typedef struct GCCPACK ttbrec_s
{
  //uint8_t unknown[TTBIDSZ] GCCPACK; // somehow at least identifes the string to the game
  uint32_t unknown1 GCCPACK;
  uint32_t unknown2 GCCPACK;
  uint32_t unknown3 GCCPACK;
  int32_t offset GCCPACK; // of this string, from start of ttb file
}
ttbrec_t GCCPACK;

#ifdef _MSC_VER
 #pragma pack(pop)
#endif

#endif // __TTBFILE_H__