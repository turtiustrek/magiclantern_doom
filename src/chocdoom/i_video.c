// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// $Log:$
//
// DESCRIPTION:
//	DOOM graphics stuff for X11, UNIX.
//
//-----------------------------------------------------------------------------

static const char
    rcsid[] = "$Id: i_x.c,v 1.6 1997/02/03 22:45:10 b1 Exp $";

#include "config.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_event.h"
#include "d_main.h"
#include "i_video.h"
#include "i_scale.h"
#include "z_zone.h"

#include "tables.h"
#include "doomkeys.h"
#include "bmp.h"
#include <stdint.h>
#include <stdbool.h>
#include "extfunctions.h"
#include "w_wad.h"
#include "deh_str.h"
// The screen buffer; this is modified to draw things to the screen

byte *I_VideoBuffer = NULL;

//scaled buffer
byte *S_VideoBuffer = NULL;

// If true, game is running as a screensaver

boolean screensaver_mode = false;

// Flag indicating whether the screen is currently visible:
// when the screen isnt visible, don't render the screen

boolean screenvisible;

// Mouse acceleration
//
// This emulates some of the behavior of DOS mouse drivers by increasing
// the speed when the mouse is moved fast.
//
// The mouse input values are input directly to the game, but when
// the values exceed the value of mouse_threshold, they are multiplied
// by mouse_acceleration to increase the speed.

int button_event;
bool button_state;

float mouse_acceleration = 2.0;
int mouse_threshold = 10;

// Gamma correction level to use

int usegamma = 0;

int usemouse = 0;

// If true, keyboard mapping is ignored, like in Vanilla Doom.
// The sensible thing to do is to disable this if you have a non-US
// keyboard.

int vanilla_keyboard_mapping = true;

typedef struct
{
    byte r;
    byte g;
    byte b;
} col_t;

// Palette converted to RGB32

static uint32_t rgb32_palette[256];

// Last touch state

// Last button state

static bool last_button_state;

// run state

static bool run;
struct marv
{
    uint32_t signature;    // MARV - VRAM reversed
    uint8_t *bitmap_data;  // either UYVY or UYVY + opacity
    uint8_t *opacity_data; // optional; if missing, interleaved in bitmap_data
    uint32_t flags;        // (flags is a guess)
    uint32_t width;        // X resolution; may be larger than screen size
    uint32_t height;       // Y resolution; may be larger than screen size
    uint32_t pmem;         // pointer to PMEM (Permanent Memory) structure
};

void I_InitGraphics(void)
{

    I_VideoBuffer = (byte *)Z_Malloc(SCREENWIDTH * SCREENHEIGHT, PU_STATIC, NULL);
    S_VideoBuffer = (byte *)Z_Malloc(mode_scale_2x.height * mode_scale_2x.width, PU_STATIC, NULL);
    uart_printf("S_VideoBuffer at: 0x%x X:%d Y:%d\n", S_VideoBuffer, mode_scale_2x.width, mode_scale_2x.height);

    screenvisible = true;
}

void I_ShutdownGraphics(void)
{
    Z_Free(S_VideoBuffer);
    Z_Free(I_VideoBuffer);
}

void I_StartFrame(void)
{
}

void I_GetEvent(void)
{
    event_t event;
    bool local_button_state;

    local_button_state = button_state; //turtius: implement button reads

    if (last_button_state != button_state)
    {
        last_button_state = local_button_state;
        event.type = last_button_state ? ev_keydown : ev_keyup;
        event.data1 = button_event;
        event.data2 = -1;
        event.data3 = -1;
        button_state =0;
        D_PostEvent(&event);
    }
}

void I_StartTic(void)
{
    I_GetEvent();
}

void I_UpdateNoBlit(void)
{
}

extern void RefreshVrmsSurface(void);
//7% increese in speed with unrolling and stuff
//TODO: find a better way to refill the VRAM buffer

extern void GrypFill3DRect(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j, int k, int l, int m);
extern void JediDraw(int a, int *b, int c, int *d);
extern void GrypDrawHLine(int a, int b, int c, int d);
extern void GrypDrawLine(int marv, int x0, int y0, int x1, int y1);
extern int GrypDrawImage(int marv, int pointer, int unkn, int unkn1, int x, int y, int yy);
extern void GrypFinish(int marv, int a, int b, int c, int d, int e);
extern int GrypDrawRect(int marv, int a, int b, int c, int d, int e, int f, int g, int h);
extern int GrypSetDstVramHandle(int marv);
extern void GrypSetGradStep(int a, int b, int c, int d, int e, int f, int g, int h);
extern void XimrExe(int a);
extern void DrawVRAM(int a, int b, int c, int d, int e, int f, int g, int h, int i);
void I_FinishUpdate(void)
{
    // int start = MEM(0xD400000C);
    int x, y;
    byte index;
    struct MARV *rgb_MARV = MEM(0xfd80);
    uint32_t *bgra = rgb_MARV->bitmap_data;
    I_InitScale(I_VideoBuffer, S_VideoBuffer, mode_scale_2x.width * 1);

    mode_scale_2x.DrawScreen(0, 0, SCREENWIDTH, SCREENHEIGHT);

    int row = 0;
    int col = 0;

    for (y = 0; y < mode_scale_2x.height; y++)
    {
        row = y * mode_scale_2x.width;
        col = ((y + 50) * 960);
        for (x = 0; x < mode_scale_2x.width; x += 8)
        { //stolen from names_are_hard and kitor
            *(bgra + ((x + 150)) + (col)) = rgb32_palette[S_VideoBuffer[row + x]];
            *(bgra + ((x + 151)) + (col)) = rgb32_palette[S_VideoBuffer[row + x + 1]];
            *(bgra + ((x + 152)) + (col)) = rgb32_palette[S_VideoBuffer[row + x + 2]];
            *(bgra + ((x + 153)) + (col)) = rgb32_palette[S_VideoBuffer[row + x + 3]];
            *(bgra + ((x + 154)) + (col)) = rgb32_palette[S_VideoBuffer[row + x + 4]];
            *(bgra + ((x + 155)) + (col)) = rgb32_palette[S_VideoBuffer[row + x + 5]];
            *(bgra + ((x + 156)) + (col)) = rgb32_palette[S_VideoBuffer[row + x + 6]];
            *(bgra + ((x + 157)) + (col)) = rgb32_palette[S_VideoBuffer[row + x + 7]];
        }
    }

    XimrExe(0xA09A0);
}

//
// I_ReadScreen
//
void I_ReadScreen(byte *scr)
{
    memcpy(scr, I_VideoBuffer, SCREENWIDTH * SCREENHEIGHT);
}

//
// I_SetPalette
//
void I_SetPalette(byte *palette)
{
    int i;
    col_t *c;

    for (i = 0; i < 256; i++)
    {
        c = (col_t *)palette;
        rgb32_palette[i] = (gammatable[usegamma][c->b] & 0xff) | (gammatable[usegamma][c->g] << 8) | (gammatable[usegamma][c->r] << 16) | (0xff << 24);
        //rgb565_palette[i] = GFX_RGB565(gammatable[usegamma][c->r],
        //							   gammatable[usegamma][c->g],
        //							   gammatable[usegamma][c->b]);
        //turtius: fix this
        palette += 3;
    }
}

// Given an RGB value, find the closest matching palette index.

int I_GetPaletteIndex(int r, int g, int b)
{
    int best, best_diff, diff;
    int i;
    col_t color;

    best = 0;
    best_diff = INT_MAX;

    for (i = 0; i < 256; ++i)
    {
        color.b = rgb32_palette[i] & 0xff;
        color.g = (rgb32_palette[i] >> 8) & 0xff;
        color.r = (rgb32_palette[i] >> 16) & 0xff;
        //turtius: fix this
        diff = (r - color.r) * (r - color.r) + (g - color.g) * (g - color.g) + (b - color.b) * (b - color.b);

        if (diff < best_diff)
        {
            best = i;
            best_diff = diff;
        }

        if (diff == 0)
        {
            break;
        }
    }

    return best;
}

void I_BeginRead(void)
{
}

void I_EndRead(void)
{
}

void I_SetWindowTitle(char *title)
{
}

void I_GraphicsCheckCommandLine(void)
{
}

void I_SetGrabMouseCallback(grabmouse_callback_t func)
{
}

void I_EnableLoadingDisk(void)
{
}

void I_BindVideoVariables(void)
{
}

void I_DisplayFPSDots(boolean dots_on)
{
}

void I_CheckIsScreensaver(void)
{
}
