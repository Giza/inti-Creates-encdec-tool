//
// portable memory mapped files
//

#include <stddef.h>
#include <malloc.h>
#include <stdint.h>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <string.h>
  #include <unistd.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <fcntl.h>
  #include <sys/mman.h>
#endif

#include "mmfiles.h"

#ifdef _WIN32

// read only, copy on write, no commit back to disk
mmapinfo_t *mmap_existing_read_cow(char *filename)
{
  HANDLE filehandle;
  HANDLE maphandle;
  LPVOID mapptr;
  DWORD filesize;
  mmapinfo_t *mnfo;
  
  filehandle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (filehandle == INVALID_HANDLE_VALUE)
    return NULL;
    
  filesize = GetFileSize(filehandle, NULL);

  maphandle = CreateFileMapping(filehandle, NULL, PAGE_READONLY, 0, 0, NULL);
  if (!maphandle)
  {
    CloseHandle(filehandle);
    return NULL;
  }
  
  mapptr = MapViewOfFile(maphandle, FILE_MAP_COPY, 0,0,0);
  if (!mapptr)
  {
    CloseHandle(maphandle);
    CloseHandle(filehandle); 
    return NULL;
  }

  mnfo = malloc(sizeof(mmapinfo_t));
  if (!mnfo)
  {
    UnmapViewOfFile(mapptr);
    CloseHandle(maphandle);
    CloseHandle(filehandle);
    return NULL;
  }
  
  mnfo->fh = filehandle;
  mnfo->mh = maphandle;
  mnfo->size = filesize;
  mnfo->ptr = mapptr;
  mnfo->path = strdup(filename);
  
  return mnfo;
}

mmapinfo_t *mmap_create_overwrite(char *filename, int size)
{
  HANDLE filehandle;
  HANDLE maphandle;
  LPVOID mapptr;
  mmapinfo_t *mnfo;
  
  filehandle = CreateFile(filename, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (filehandle == INVALID_HANDLE_VALUE)
    return NULL;
    
  maphandle = CreateFileMapping(filehandle, NULL, PAGE_READWRITE, 0, size, NULL);
  if (!maphandle)
  {
    CloseHandle(filehandle);
    return NULL;
  }
  
  mapptr = MapViewOfFile(maphandle, FILE_MAP_WRITE, 0,0,0);
  if (!mapptr)
  {
    CloseHandle(maphandle);
    CloseHandle(filehandle); 
    return NULL;
  }

  mnfo = malloc(sizeof(mmapinfo_t));
  if (!mnfo)
  {
    UnmapViewOfFile(mapptr);
    CloseHandle(maphandle);
    CloseHandle(filehandle);
    return NULL;
  }

  mnfo->fh = filehandle;
  mnfo->mh = maphandle;
  mnfo->size = size;
  mnfo->ptr = mapptr;
  mnfo->path = strdup(filename);

  return mnfo;
}
                     
void close_mmapping(mmapinfo_t *mnfo)
{
  UnmapViewOfFile(mnfo->ptr);
  CloseHandle(mnfo->mh);
  CloseHandle(mnfo->fh);
  free(mnfo->path);
  free(mnfo);
}

#else // not _WIN32

mmapinfo_t *mmap_existing_read_cow(char *filename)
{
  int fd;
  struct stat statbuf;
  size_t filesize;
  void *mapptr;
  mmapinfo_t *mnfo;

  fd = open(filename, O_RDONLY);
  if (fd < 0)
    return NULL;

  if (fstat(fd, &statbuf) < 0)
  {
    close(fd);
    return NULL;
  }

  filesize = statbuf.st_size;

  mapptr = mmap(NULL, filesize, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
  if (mapptr < 0)
  {
    close(fd);
    return NULL;
  }

  mnfo = malloc(sizeof(mmapinfo_t));
  if (!mnfo)
  {
    munmap(mapptr, filesize);
    close(fd);
    return NULL;
  }

  mnfo->fh = fd;
  mnfo->size = filesize;
  mnfo->ptr = mapptr;
  mnfo->path = strdup(filename);

  return mnfo;  
}

#define BLOCKSIZE (128*1024)
static uint8_t nullbuf[BLOCKSIZE];
static int nullbuf_initialized = 0;

mmapinfo_t *mmap_create_overwrite(char *filename, int size)
{
  int fd, i;
  void *mapptr;
  mmapinfo_t *mnfo;

  //chmod(mnfo->path, 0777);
  //unlink(mnfo->path);

  fd = open(filename, O_RDWR|O_TRUNC|O_CREAT);
  if (fd < 0)
    return NULL;
    
  // is there seriously no better way to do this on not-windows?!?!
  // (open a mapping that is longer than the actual existing file on disk)
  if (!nullbuf_initialized)
  {
    memset(nullbuf, 0, BLOCKSIZE);
    nullbuf_initialized = 1;
  }
  
  if (size < BLOCKSIZE)
    write(fd, nullbuf, size);
  else
  {
    for (i=0; i<size/BLOCKSIZE; i++)
      write(fd, nullbuf, BLOCKSIZE);
    
    write(fd, nullbuf, size%BLOCKSIZE);
  }

  mapptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if (mapptr < 0)
  {
    close(fd);
    return NULL;
  }

  mnfo = malloc(sizeof(mmapinfo_t));
  if (!mnfo)
  {
    munmap(mapptr, size);
    close(fd);
    return NULL;
  }

  mnfo->fh = fd;
  mnfo->size = size;
  mnfo->ptr = mapptr;
  mnfo->path = strdup(filename);
  
  return mnfo;  
}

void close_mmapping(mmapinfo_t *mnfo)
{
  munmap(mnfo->ptr, mnfo->size);
  close(mnfo->fh);
  chmod(mnfo->path, 0644);
  free(mnfo->path);
  free(mnfo);
}

#endif // _WIN32
