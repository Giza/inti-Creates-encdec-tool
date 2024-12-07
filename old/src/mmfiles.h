//
// portable memory mapped files, header
//

#ifndef __MMFILES_H__
#define __MMFILES_H__

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <fcntl.h>
  #include <sys/mman.h>
#endif

#ifdef _WIN32
  typedef struct
  {
    HANDLE fh;
    HANDLE mh;
    DWORD size;
    LPVOID ptr;
    char *path;
  }
  mmapinfo_t;
#else // not _WIN32
  typedef struct
  {
    int fh;
    size_t size;
    void *ptr;
    char *path;
  }
  mmapinfo_t;
#endif

extern mmapinfo_t *mmap_existing_read_cow(char *filename); // read only, copy on write, no commit back to disk
extern mmapinfo_t *mmap_create_overwrite(char *filename, int size); // read/write, create new file or truncate/overwrite existing

extern void close_mmapping(mmapinfo_t *mnfo);

#endif // __MMFILES_H__
