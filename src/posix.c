
#include <dryos.h>
#include "extfunctions.h"
#include <mem.h>
#include <rand.h>

/* in this file we implement everything needed which is meant to make 
   Magic Lantern compatible to POSIX if needed.
   For a list of POSIX headers and the prototypes in it, see for example:
   http://pubs.opengroup.org/onlinepubs/9699919799/idx/head.html
   
   This does *not* mean, ML has to implement a whole POSIX system, but
   this is merely the file where will place missing functions that are
   defined in POSIX but not provided by our libc version or Canon's OS.
*/

/**
 * POSIX standard assumes rand() to return always positive integers
 * but we may return negative ones when casting an uint32_t to int
 */
int rand()
{
    uint32_t ret = 0;
    
    rand_fill(&ret, 1);

    // Clear sign bit
    return ret & 0x7fffffff; 
}

void srand(unsigned int seed)
{
    rand_seed(seed);
}

char *strdup(const char*str)
{
    char *ret = _AllocateMemory(strlen(str) + 1);
    strcpy(ret, str);
    return ret;
}

/* warning - not implemented yet */
int time()
{
    return 0;
}

int clock()
{
    return rand();
}

void *calloc(size_t nmemb, size_t size)
{
    void *ret = _AllocateMemory(nmemb * size);
    if (ret)
        memset(ret, 0x00, nmemb * size);
    
    return ret;
}

void *realloc(void *ptr, size_t size)
{
    void *ret = _AllocateMemory(size);
    
    if (ptr)
    {
        memcpy(ret, ptr, size);
        _FreeMemory(ptr);
    }
    
    return ret;
}

size_t strlcat(char *dest, const char *src, size_t n)
{
    uint32_t dst_len = strlen(dest);
    uint32_t src_len = strlen(src);
    uint32_t len = MIN(n - dst_len - 1, src_len);
    
    memcpy(&dest[dst_len], src, len);
    dest[dst_len + len] = '\000';
    
    return dst_len + len;
}


char *strcat(char *dest, const char *src)
{
    strlcat(dest, src, 0x7FFFFFFF);
    return dest;
}

int strncasecmp(const char *s1, const char *s2, int n)
{
    if (n && s1 != s2)
    {
        do {
            int d = tolower(*s1) - tolower(*s2);
            if (d || *s1 == '\0' || *s2 == '\0') return d;
            s1++;
            s2++;
        } while (--n);
    }
    return 0;
}
float atof(const char* s){
  float rez = 0, fact = 1;
  if (*s == '-'){
    s++;
    fact = -1;
  };
  for (int point_seen = 0; *s; s++){
    if (*s == '.'){
      point_seen = 1; 
      continue;
    };
    int d = *s - '0';
    if (d >= 0 && d <= 9){
      if (point_seen) fact /= 10.0f;
      rez = rez * 10.0f + (float)d;
    };
  };
  return rez * fact;
};


