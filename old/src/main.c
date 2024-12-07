#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <zlib.h>

#ifndef _MSC_VER
 #include <strings.h>
 #define stricmp strcasecmp
 #include <sys/stat.h>
#endif

#include "mmfiles.h"
#include "encdec.h"

typedef uint8_t byte;

enum
{
  COMP_NO,
  COMP_YES,
  COMP_REVERSE
};

typedef struct
{
  const char *shorthand;
  const char *password1;
  const char *password2;
  const int compressed;
  const int headerskip;
  const int need_steamid;
}
inti_filetype_t;

const inti_filetype_t FileTypes[] =
{
  { "bft",    "bft90210",NULL,     COMP_YES,    0, 0 }, // BMPFont*.bfb files
  { "obj",    "obj90210",NULL,     COMP_YES,    0, 0 }, // *.osb
  { "scroll", "scroll90210",NULL,  COMP_YES,    0, 0 }, // stage "scroll" (background) files, *.scb
  { "set",    "set90210",NULL,     COMP_NO,     0, 0 }, // stage "set" (setup?) files, *.stb
  { "snd",    "snd90210",NULL,     COMP_NO,     0, 0 }, // *.bisar (some kind of index or keys to sound data .bigrp files?)
  { "txt",    "txt20170401",NULL,  COMP_YES,    0, 0 }, // text resource files, *.ttb
  { "txt2",   "x4NKvf3U",NULL,     COMP_YES,    0, 0 }, // v2 text resource files, *.tb2
  
  { "json",   "json180601",NULL,   COMP_YES,     0, 0 }, // haven't found any yet, doesn't work for JSON files from DMFD PC
  { "json2",  "xN5sUeRo",NULL,     COMP_REVERSE, 0, 0 }, // JSON files from DMFD PC, data is scrambled *before* it is zlib compressed!
  
  { "save1", "gYjkJoTX","zZ2c9VTK",  COMP_NO, 16, 0 },  // system and game save data files from COTM1 and COTM2, scrambled twice with two
  { "save2", "gVTYZ2jk","JoTXzc9K",  COMP_NO, 16, 0 },  // different passwords/keys, also there's a 16-byte header which IS NOT scrambled.
  
  { "save3", "gYjkJoTX","zZ2c9VTK",  COMP_NO, 16, 1 },  // DMFD PC save data, same deal, two passwords, untouched header, but the passwords
													     // have a tail appended that is based on the player's Steam ID (least significant
													     // 32 bits of it in hex)
  
  { "ssbpi",  "ssbpi90210",NULL,     COMP_NO, 0, 0 }, // ??? may be compressed as well, or may not even exist anywhere!

   { NULL, NULL, NULL, 0, 0, 0 }
};

int CountFileTypes (void)
{
  int i;

  i = 0;
  while (FileTypes[i].shorthand != NULL) i++;

  return i;
}

void PrintFileTypes (void)
{
  int i, count;

  printf("predefined filetypes:\n");
  count = CountFileTypes();
  for (i=0; i<count; i++)
    printf("%s%s", FileTypes[i].shorthand, FileTypes[i].need_steamid ? "* " : " ");

  printf("\n");
}

int GetIntiFileType (char *search)
{
  int i, count;

  count = CountFileTypes();
  for (i=0; i<count; i++)
  {
    if (!stricmp(FileTypes[i].shorthand, search))
      return i;
  }

  printf("bad file type '%s' specified\n", search);
  PrintFileTypes();
  exit(-1);
}

void ShowUsage (void)
{
  printf("\n"\
         "usage: inti_encdec <d/e> <filetype> [steamid] <infile> <outfile>\n"\
         "        decode/encode using a predefined filetype\n"\
		 "\n"\
		 "        Save files from certain games may require your SteamID to\n"\
		 "        descramble/scramble! Input it after the file type if necessary.\n"\
		 "        The filetypes which require your steamid are marked with a '*'.\n"\
         /*"\n"\
         "        inti_encdec <lt>\n"\
         "        list predefined filetypes\n"\
         "\n"\*/
         /*"        inti_encdec <md/me> <comp> <hdr> <keyinfo> <infile> <outfile>\n"\
         "        decode/encode with manually specified parameters:\n"\
         "         <comp> = try to zlib decompress/compress data?\n"\
         "                   0: no compression\n"\
         "                   1: first compressed, then scrambled\n"\
         "                   2: first scrambled, then compressed (eg. DMFD PC JSONs)\n"\
         "         <hdr> = how many header bytes to leave unscrambled? 0 for none\n"\
         "         <keyinfo> = ...\n"\*/
         "\n");

  PrintFileTypes();      
  exit(-1);
}

enum
{
  MODE_DEC,
  MODE_ENC
};

int main (int argc, char **argv)
{
  char *command;

  int r;
  int mode; // 0=decoding, 1=encoding
  int compressed, headerskip;
  uint64_t key1, key2;

  mmapinfo_t *mminfile, *mmoutfile;
  //int inlength, outlength, zlength;
  byte *inbuf, *outbuf, *zbuf;

  char *inpath, *outpath;


  if (argc < 5)
    ShowUsage();

  command = argv[1];

  // decode or encode using a predefined filetype
  if (!stricmp(command, "d") || !stricmp(command,"e"))
  {
    int typeindex;
    const inti_filetype_t *predef;

    predef = &FileTypes[GetIntiFileType(argv[2])];

    compressed = predef->compressed;
    headerskip = predef->headerskip;
	
    if (predef->need_steamid)
	{
	  uint64_t steamid;
	  uint32_t truncated;
	  char pwdbuf[64];
	  
	  if (argc < 6)
	    ShowUsage();
	  
	  r = sscanf(argv[3], "%"SCNu64, &steamid);
	  if (r != 1 || !steamid)
	  {
	    printf("bad steamid specified (sscanf failed)\n");
		return -1;
	  }
	  
	  truncated = steamid; // & 0x00000000FFFFFFFF
	  snprintf(pwdbuf, 64, "%s%x", predef->password1, truncated);
	  key1 = inti_keygen(pwdbuf);

      if (predef->password2)
	  {
        snprintf(pwdbuf, 64, "%s%x", predef->password2, truncated);
		key2 = inti_keygen(pwdbuf);
	  }
      else
        key2 = 0;

      inpath = argv[4];
      outpath = argv[5];
	}
	else
	{
	  key1 = inti_keygen(predef->password1);

      if (predef->password2)
        key2 = inti_keygen(predef->password2);
      else
        key2 = 0;

      inpath = argv[3];
      outpath = argv[4];
	}

    if (!stricmp(command, "e"))
      mode = MODE_ENC;
    else
      mode = MODE_DEC;
  }
  // decode or encode using manually specified parameters
  else if (!stricmp(command, "md") || !stricmp(command,"me"))
  {
    printf("not implemented yet!");
    exit(-1);
  }
  else
    ShowUsage();

  if (compressed && headerskip)
  {
    printf("you cannot specify a header skip and enable compression at the same time\n");
    return -1;
  }

  mminfile = mmap_existing_read_cow(inpath);
  if (!mminfile)
  {
    printf("failed to memory map input file '%s'\n", inpath);
    return -1;
  }

  if (mode == MODE_DEC)
  {
    if (!compressed)
    {
	  printf("descrambling...\r"); fflush(stdout);
	  
	  inti_encdec((byte*)mminfile->ptr+headerskip, ENCDEC_MODE_DEC, mminfile->size-headerskip, key1);
      if (key2)
        inti_encdec((byte*)mminfile->ptr+headerskip, ENCDEC_MODE_DEC, mminfile->size-headerskip, key2);
	
	  printf("descrambled %i bytes\n", mminfile->size-headerskip);
	  

      mmoutfile = mmap_create_overwrite(outpath, mminfile->size);
      if (!mmoutfile)
      {
        printf("failed to memory map output file '%s'\n", outpath);
        return -1;
      }

      memcpy(mmoutfile->ptr, mminfile->ptr, mminfile->size);
      close_mmapping(mmoutfile);
    }
    else if (compressed == COMP_REVERSE) // zlib decompress first, then descramble (DMFD PC JSONs)
    {
      uint32_t unzsize;

      // first 4 bytes is uncompressed length
      memcpy(&unzsize, mminfile->ptr, sizeof(uint32_t));

      // now that length is known, create output mmapped file
      mmoutfile = mmap_create_overwrite(outpath, unzsize);
      if (!mmoutfile)
      {
        printf("failed to memory map output file '%s'\n", outpath);
        return -1;
      }

      printf("uncompressing...\r"); fflush(stdout);
	  
	  r = uncompress(mmoutfile->ptr, (uLongf*)&unzsize, (byte*)mminfile->ptr+sizeof(uint32_t), mminfile->size-sizeof(uint32_t));
      if (r != Z_OK)
      {
        printf("zlib decompression failed, error code %i\n", r);
        return -1;
      }
	  
	  printf("uncompressed %i => %i bytes\n", mminfile->size-sizeof(uint32_t), unzsize);	  

      printf("descrambling...\r"); fflush(stdout);
	  
	  inti_encdec(mmoutfile->ptr, ENCDEC_MODE_DEC, unzsize, key1);
      if (key2)
        inti_encdec(mmoutfile->ptr, ENCDEC_MODE_DEC, unzsize, key2);

      printf("descrambled %i bytes\n", unzsize);
	  
      close_mmapping(mmoutfile);
    }
    else // descramble first, then zlib decompress (everything else so far)
    {
      uint32_t unzsize;

      printf("descrambling...\r"); fflush(stdout);
	  
	  inti_encdec(mminfile->ptr, ENCDEC_MODE_DEC, mminfile->size, key1);
      if (key2)
        inti_encdec(mminfile->ptr, ENCDEC_MODE_DEC, mminfile->size, key2);
	
	  printf("descrambled %i bytes\n", mminfile->size);

      memcpy(&unzsize, mminfile->ptr, sizeof(uint32_t));

      mmoutfile = mmap_create_overwrite(outpath, unzsize);
      if (!mmoutfile)
      {
        printf("failed to memory map output file '%s'\n", outpath);
        return -1;
      }
	  
	  printf("uncompressing...\r"); fflush(stdout);

      r = uncompress(mmoutfile->ptr, (uLongf*)&unzsize, (byte*)mminfile->ptr+sizeof(uint32_t), mminfile->size-sizeof(uint32_t));
      if (r != Z_OK)
      {
        printf("zlib decompression failed, error code %i\n", r);
        return -1;
      }
	  
	  printf("uncompressed %i => %i bytes\n", mminfile->size-sizeof(uint32_t), unzsize);

      close_mmapping(mmoutfile);
    }
  }
  else // mode ENC(ode)
  {
    if (!compressed)
    {
      printf("scrambling...\r"); fflush(stdout);
	  
	  inti_encdec((byte*)mminfile->ptr+headerskip, ENCDEC_MODE_ENC, mminfile->size-headerskip, key1);
      if (key2)
        inti_encdec((byte*)mminfile->ptr+headerskip, ENCDEC_MODE_ENC, mminfile->size-headerskip, key2);
	  
	  printf("scrambled %i bytes\n", mminfile->size-headerskip);

      mmoutfile = mmap_create_overwrite(outpath, mminfile->size);
      if (!mmoutfile)
      {
        printf("failed to memory map output file '%s'\n", outpath);
        return -1;
      }

      memcpy(mmoutfile->ptr, mminfile->ptr, mminfile->size);
      close_mmapping(mmoutfile);
    }
    else if (compressed == COMP_REVERSE) // first scramble, then compress (DMFD PC JSON files)
    {
      uint32_t zsize;
      
      printf("scrambling...\r"); fflush(stdout);
	  
	  inti_encdec(mminfile->ptr, ENCDEC_MODE_ENC, mminfile->size, key1);
      if (key2)
        inti_encdec(mminfile->ptr, ENCDEC_MODE_ENC, mminfile->size, key2);
		
      printf("scrambled %i bytes\n", mminfile->size);
      
      zsize = compressBound(mminfile->size);
      zbuf = malloc(zsize + sizeof(uint32_t)); // leave space for header
      if (!zbuf)
      {
        printf("failed to allocate memory for zlib compression buffer\n");
        return -1;
      }

      printf("compressing...\r"); fflush(stdout);
	  
	  r = compress2(zbuf+sizeof(uint32_t), (uLongf*)&zsize, mminfile->ptr, mminfile->size, 9);
      if (r != Z_OK)
      {
        printf("zlib compression failed, error code %i\n", r);
        return -1;
      }
	  
	  printf("compressed %i => %i bytes\n", mminfile->size, zsize);

      // add header for decompressed size
      memcpy(zbuf, &mminfile->size, sizeof(uint32_t));

      mmoutfile = mmap_create_overwrite(outpath, zsize+sizeof(uint32_t));
      if (!mmoutfile)
      {
        printf("failed to memory map output file '%s'\n", outpath);
        return -1;
      }

      memcpy(mmoutfile->ptr, zbuf, zsize+sizeof(uint32_t));
      close_mmapping(mmoutfile);
      free(zbuf);
    }
    else // first compress, then scramble (everything else...)
    {
      uint32_t zsize;

      zsize = compressBound(mminfile->size);
      zbuf = malloc(zsize + sizeof(uint32_t)); // leave space for header
      if (!zbuf)
      {
        printf("failed to allocate memory for zlib compression buffer\n");
        return -1;
      }

      printf("compressing...\r"); fflush(stdout);
	  
	  r = compress2(zbuf+sizeof(uint32_t), (uLongf*)&zsize, mminfile->ptr, mminfile->size, 9);
      if (r != Z_OK)
      {
        printf("zlib compression failed, error code %i\n", r);
        return -1;
      }
	  
	  printf("compressed %i => %i bytes\n", mminfile->size, zsize);
	  
	  // add header for decompressed size
	  memcpy(zbuf, &mminfile->size, sizeof(uint32_t));

      printf("scrambling...\r"); fflush(stdout);
	  
	  inti_encdec(zbuf, ENCDEC_MODE_ENC, zsize+sizeof(uint32_t), key1);
      if (key2)
        inti_encdec(zbuf, ENCDEC_MODE_ENC, zsize+sizeof(uint32_t), key2);
		
	  printf("scrambled %i bytes\n", zsize+sizeof(uint32_t));
      
      mmoutfile = mmap_create_overwrite(outpath, zsize+sizeof(uint32_t));
      if (!mmoutfile)
      {
        printf("failed to memory map output file '%s'\n", outpath);
        return -1;
      }

      memcpy(mmoutfile->ptr, zbuf, zsize+sizeof(uint32_t));
      close_mmapping(mmoutfile);
      free(zbuf);
    }
  }

  close_mmapping(mminfile);    
  return 0;
}
