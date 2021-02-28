#include "dryos.h"


#include "string.h"
#include "menu.h"

#include "extfunctions.h"
#include "fio-ml.h"

static void fixup_filename(char* new_filename, const char* old_filename, int size)
{

#define IS_IN_ML_DIR(filename)   (strncmp("ML/", filename, 3) == 0)
#define IS_IN_ROOT_DIR(filename) (filename[0] == '/' || !strchr(filename, '/'))
#define IS_DRV_PATH(filename)    (filename[1] == ':')
    char* drive_letter = "B";
   
    if (IS_DRV_PATH(old_filename))
    {
        strncpy(new_filename, old_filename, size-1);
        new_filename[size-1] = '\0';
        uart_printf("IS_DRV_PATH (%s)\n",new_filename);
        return;
    }

    if (!(IS_IN_ML_DIR(old_filename) || IS_IN_ROOT_DIR(old_filename)))
    {
        drive_letter = "B";
    }
    snprintf(new_filename, size, "%s:/%s", drive_letter, old_filename);
     uart_printf("(%s)\n",new_filename);
#undef IS_IN_ML_DIR
#undef IS_IN_ROOT_DIR
#undef IS_DRV_PATH

}


/* Canon stub */
/* note: it returns -1 on error, unlike fopen from plain C */
FILE* _FIO_OpenFile(const char* filename, unsigned mode );

/* this one returns 0 on error, just like in plain C */
FILE* FIO_OpenFile(const char* filename, unsigned mode )
{
    uart_printf("FIO_OpenFile called!\n");
    char new_filename[FIO_MAX_PATH_LENGTH];
    fixup_filename(new_filename, filename, sizeof(new_filename));
    
    FILE* f = _FIO_OpenFile(new_filename, mode);
    
    if (f != PTR_INVALID)
    {
        return f;
    }
    
    return 0;
}

int _FIO_GetFileSize(const char * filename, uint32_t * size);
int FIO_GetFileSize(const char * filename, uint32_t * size)
{
    char new_filename[FIO_MAX_PATH_LENGTH];
    fixup_filename(new_filename, filename, sizeof(new_filename));
    return _FIO_GetFileSize(new_filename, size);
}

int _FIO_RemoveFile(const char * filename);
int FIO_RemoveFile(const char * filename)
{
    char new_filename[FIO_MAX_PATH_LENGTH];
    fixup_filename(new_filename, filename, sizeof(new_filename));
    return _FIO_RemoveFile(new_filename);
}

struct fio_dirent * _FIO_FindFirstEx(const char * dirname, struct fio_file * file);
struct fio_dirent * FIO_FindFirstEx(const char * dirname, struct fio_file * file)
{
    char new_dirname[FIO_MAX_PATH_LENGTH];
    fixup_filename(new_dirname, dirname, sizeof(new_dirname));
    return _FIO_FindFirstEx(new_dirname, file);
}

int _FIO_CreateDirectory(const char * dirname);
int FIO_CreateDirectory(const char * dirname)
{
    char new_dirname[FIO_MAX_PATH_LENGTH];
    fixup_filename(new_dirname, dirname, sizeof(new_dirname));
    if (is_dir(new_dirname)) return 0;
    return _FIO_CreateDirectory(new_dirname);
}

#if defined(CONFIG_FIO_RENAMEFILE_WORKS)
int _FIO_RenameFile(const char * src, const char * dst);
int FIO_RenameFile(const char * src, const char * dst)
{
    char newSrc[FIO_MAX_PATH_LENGTH];
    char newDst[FIO_MAX_PATH_LENGTH];
    fixup_filename(newSrc, src, FIO_MAX_PATH_LENGTH);
    fixup_filename(newDst, dst, FIO_MAX_PATH_LENGTH);
    return _FIO_RenameFile(newSrc, newDst);
}
#else
int FIO_RenameFile(const char * src, const char * dst)
{
    // FIO_RenameFile not known, or doesn't work
    // emulate it by copy + erase (poor man's rename :P )
    return FIO_MoveFile(src, dst);
}
#endif

static unsigned _GetFileSize(char* filename)
{
    uint32_t size;
    if( _FIO_GetFileSize( filename, &size ) != 0 )
        return 0xFFFFFFFF;
    return size;
}

uint32_t FIO_GetFileSize_direct(const char* filename)
{
    char new_filename[FIO_MAX_PATH_LENGTH];
    fixup_filename(new_filename, filename, sizeof(new_filename));
    return _GetFileSize(new_filename);
}

static void _FIO_CreateDir_recursive(char* path)
{
    //~ NotifyBox(2000, "create dir: %s ", path); msleep(2000);
    // B:/ML/something
    
    if (is_dir(path)) return;

    int n = strlen(path);
    for (int i = n-1; i > 2; i--)
    {
        if (path[i] == '/')
        {
            path[i] = '\0';
            if (!is_dir(path))
                _FIO_CreateDir_recursive(path);
            path[i] = '/';
        }
    }

    _FIO_CreateDirectory(path);
}

/* Canon stub */
/* note: it returns -1 on error, unlike fopen from plain C */
FILE* _FIO_CreateFile(const char* filename );

/* a wrapper that also creates missing dirs and removes existing file */
/* this one returns 0 on error, just like in plain C */
static FILE* _FIO_CreateFileEx(const char* name)
{
    // first assume the path is alright
    _FIO_RemoveFile(name);
    FILE* f = _FIO_CreateFile(name);

    if (f != PTR_INVALID)
    {
        return f;
    }

    // if we are here, the path may be inexistent => create it
    int n = strlen(name);
    char* namae = (char*) name; // trick to ignore the const declaration and split the path easily
    for (int i = n-1; i > 2; i--)
    {
         if (namae[i] == '/')
         {
             namae[i] = '\0';
             _FIO_CreateDir_recursive(namae);
             namae[i] = '/';
         }
    }

    f = _FIO_CreateFile(name);

    if (f != PTR_INVALID)
    {
        return f;
    }
    
    /* return 0 on error, just like in plain C */
    return 0;
}

FILE* FIO_CreateFile(const char* name)
{
    char new_name[FIO_MAX_PATH_LENGTH];
    fixup_filename(new_name, name, sizeof(new_name));
    return _FIO_CreateFileEx(new_name);
}

/* Canon stubs */
extern int _FIO_ReadFile( FILE* stream, void* ptr, size_t count );
extern int _FIO_WriteFile( FILE* stream, const void* ptr, size_t count );

int FIO_ReadFile( FILE* stream, void* ptr, size_t count )
{
    if (ptr == CACHEABLE(ptr))
    {
        /* there's a lot of existing code (e.g. mlv_play) that's hard to refactor */
        /* workaround: allocate DMA memory here (for small buffers only)
         * code that operates on large buffers should be already correct */
        ASSERT(count <= 8192);
        void * ubuf = _AllocateMemory(count);
        if (!ubuf)
        {
            ASSERT(0);
            return 0;
        }
        int ans = _FIO_ReadFile(stream, ubuf, count);
        memcpy(ptr, ubuf, count);
        _FreeMemory(ubuf);
        return ans;
    }

    return _FIO_ReadFile(stream, ptr, count);
}

int FIO_WriteFile( FILE* stream, const void* ptr, size_t count )
{
    /* we often assumed that the FIO routines will somehow care for buffers being still in cache.
       proven by the fact that even canon calls FIO_WriteFile with cached memory as source.
       that was simply incorrect. force cache flush if the buffer is a cachable one.
    */
    if (ptr == CACHEABLE(ptr))
    {
        /* write back all data to RAM */
        /* overhead is minimal (see selftest.mo for benchmark) */
        sync_caches();
    }

    return _FIO_WriteFile(stream, ptr, count);
}

FILE* FIO_CreateFileOrAppend(const char* name)
{
    /* credits: https://bitbucket.org/dmilligan/magic-lantern/commits/d7e0245b1c62c26231799e9be3b54dd77d51a283 */
    FILE * f = FIO_OpenFile(name, O_RDWR | O_SYNC);
    if (!f)
    {
        f = FIO_CreateFile(name);
    }
    else
    {
        FIO_SeekSkipFile(f,0,SEEK_END);
    }
    return f;
}

int FIO_CopyFile(const char * src, const char * dst)
{
    FILE* f = FIO_OpenFile(src, O_RDONLY | O_SYNC);
    if (!f) return -1;

    FILE* g = FIO_CreateFile(dst);
    if (!g) { FIO_CloseFile(f); return -1; }

    const int bufsize = MIN(FIO_GetFileSize_direct(src), 128*1024);
    void* buf = _AllocateMemory(bufsize);
    if (!buf) return -1;

    int err = 0;
    int r = 0;
    while ((r = FIO_ReadFile(f, buf, bufsize)))
    {
        int w = FIO_WriteFile(g, buf, r);
        if (w != r)
        {
            /* copy failed; abort and delete the incomplete file */
            err = 1;
            break;
        }
    }

    FIO_CloseFile(f);
    FIO_CloseFile(g);
    _FreeMemory(buf);
    
    if (err)
    {
        FIO_RemoveFile(dst);
        return -1;
    }
    
    /* all OK */
    return 0;
}

int FIO_MoveFile(const char * src, const char * dst)
{
    int err = FIO_CopyFile(src,dst);
    if (!err)
    {
        /* file copied, we can remove the old one */
        FIO_RemoveFile(src);
        return 0;
    }
    else
    {
        /* something went wrong; keep the old file and return error code */
        return err;
    }
}

int is_file(const char* path)
{
    uint32_t file_size = 0;
    return !FIO_GetFileSize(path, &file_size);
}

int is_dir(const char* path)
{
    struct fio_file file;
    struct fio_dirent * dirent = FIO_FindFirstEx( path, &file );
    if( IS_ERROR(dirent) )
    {
        return 0; // this dir does not exist
    }
    else 
    {
        FIO_FindClose(dirent);
        return 1; // dir found
    }
}

int get_numbered_file_name(const char* pattern, int nmax, char* filename, int maxlen)
{
    for (int num = 0; num <= nmax; num++)
    {
        snprintf(filename, maxlen, pattern, num);
        uint32_t size;
        if( FIO_GetFileSize( filename, &size ) != 0 ) return num;
        if (size == 0) return num;
    }

    snprintf(filename, maxlen, pattern, 0);
    return -1;
}

void dump_seg(void* start, uint32_t size, char* filename)
{
    DEBUG();
    FILE* f = FIO_CreateFile(filename);
    if (f)
    {
        FIO_WriteFile( f, start, size );
        FIO_CloseFile(f);
    }
    DEBUG();
}

void dump_big_seg(int k, char* filename)
{
    DEBUG();
    FILE* f = FIO_CreateFile(filename);
    if (f)
    {
        for (int i = 0; i < 16; i++)
        {
            DEBUG();
            uint32_t start = (k << 28 | i << 24);
            //bmp_printf(FONT_LARGE, 50, 50, "Saving %8x...", start);
            FIO_WriteFile( f, (const void *) start, 0x1000000 );
        }
        FIO_CloseFile(f);
    }
    DEBUG();
}


size_t
read_file(
    const char *        filename,
    void *            buf,
    size_t            size
)
{
    FILE * file = FIO_OpenFile( filename, O_RDONLY | O_SYNC );
    if (!file)
        return -1;
    unsigned rc = FIO_ReadFile( file, buf, size );
    FIO_CloseFile( file );
    return rc;
}

uint8_t* read_entire_file(const char * filename, int* buf_size)
{
    *buf_size = 0;
    uint32_t size;
    if( FIO_GetFileSize( filename, &size ) != 0 )
        goto getfilesize_fail;

    DEBUG("File '%s' size %d bytes", filename, size);

    uint8_t * buf = _AllocateMemory( size + 1);
    if( !buf )
    {
        DebugMsg( DM_MAGIC, 3, "%s: _AllocateMemory failed", filename );
        goto malloc_fail;
    }
    size_t rc = read_file( filename, buf, size );
    if( rc != size )
        goto read_fail;

    *buf_size = size;

    buf[size] = 0; // null-terminate text files

    return buf;

//~ fail_buf_copy:
read_fail:
    _FreeMemory( buf );
malloc_fail:
getfilesize_fail:
    DEBUG("failed");
    return NULL;
}

// sometimes gcc likes very much the default fprintf and uses that one
// => renamed to my_fprintf to force it to use this one
int
my_fprintf(
    FILE *          file,
    const char *        fmt,
    ...
)
{
    va_list         ap;
    int len = 0;
    
    const int maxlen = 512;
    char buf[maxlen];

    va_start( ap, fmt );
    len = vsnprintf( buf, maxlen-1, fmt, ap );
    va_end( ap );
    FIO_WriteFile( file, buf, len );
    
    return len;
}

