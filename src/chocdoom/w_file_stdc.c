//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	WAD I/O functions.
//

#include <stdio.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h> 
#include "dryos.h"

#include "m_misc.h"
#include "w_file.h"
#include "z_zone.h"
#include "i_system.h"


typedef struct
{
    wad_file_t wad;
    FILE* fstream;
} stdc_wad_file_t;

extern wad_file_class_t stdc_wad_file;

static wad_file_t *W_StdC_OpenFile(char *path)
{
#if ORIGCODE
    stdc_wad_file_t *result;
    FILE *fstream;

    fstream = fopen(path, "rb");

    if (fstream == NULL)
    {
        return NULL;
    }

    // Create a new stdc_wad_file_t to hold the file handle.

    result = Z_Malloc(sizeof(stdc_wad_file_t), PU_STATIC, 0);
    result->wad.file_class = &stdc_wad_file;
    result->wad.mapped = NULL;
    result->wad.length = M_FileLength(fstream);
    result->fstream = fstream;

    return &result->wad;
#else
    stdc_wad_file_t *result;
    FILE* file;
    file = FIO_OpenFile(path,O_RDONLY | O_SYNC);
    if (!file)
    {
         uart_printf("W_StdC_OpenFile: NULL\n");
    	return NULL;
    }
    uart_printf("W_StdC_OpenFile: (%s)  fd:(%d)\n",path,file);
    // Create a new stdc_wad_file_t to hold the file handle.

    result = Z_Malloc(sizeof(stdc_wad_file_t), PU_STATIC, 0);
	result->wad.file_class = &stdc_wad_file;
	result->wad.mapped = NULL;
	result->wad.length = M_FileLength(&file);
	result->fstream = file;
    uart_printf("E\n");
	return &result->wad;
#endif
}

static void W_StdC_CloseFile(wad_file_t *wad)
{
#if ORIGCODE
    stdc_wad_file_t *stdc_wad;

    stdc_wad = (stdc_wad_file_t *) wad;

    fclose(stdc_wad->fstream);
    Z_Free(stdc_wad);
#else
	stdc_wad_file_t *stdc_wad;

    stdc_wad = (stdc_wad_file_t *) wad;

    FIO_CloseFile(stdc_wad->fstream);
    Z_Free(stdc_wad);	
#endif
}

// Read data from the specified position in the file into the 
// provided buffer.  Returns the number of bytes read.

size_t W_StdC_Read(wad_file_t *wad, unsigned int offset,
                   void *buffer, size_t buffer_len)
{
#if ORIGCODE
    stdc_wad_file_t *stdc_wad;
    size_t result;

    stdc_wad = (stdc_wad_file_t *) wad;

    // Jump to the specified position in the file.

    fseek(stdc_wad->fstream, offset, SEEK_SET);

    // Read into the buffer.

    result = fread(buffer, 1, buffer_len, stdc_wad->fstream);

    return result;
#else
    stdc_wad_file_t *stdc_wad;
      stdc_wad = (stdc_wad_file_t *) wad;
    size_t offsetfile = FIO_SeekSkipFile(stdc_wad->fstream,offset,SEEK_SET);
    size_t result = FIO_ReadFile(stdc_wad->fstream,buffer,buffer_len);
   
      // uart_printf("W_StdC_Read offsetfiles:(0x%x) fd:(0x%x) offset:(0x%x) result:(0x%x)\n",offsetfile,stdc_wad->fstream,offset,result);

    return result;
    

#endif
}


wad_file_class_t stdc_wad_file = 
{
    W_StdC_OpenFile,
    W_StdC_CloseFile,
    W_StdC_Read,
};


