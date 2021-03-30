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
#include "dryos.h"
#include <stdint.h>
#include <stdbool.h>
#include "extfunctions.h"
#include "w_wad.h"
#include "deh_str.h"

extern void XimrExe(int a);

// The screen buffer; this is modified to draw things to the screen

byte *I_VideoBuffer = NULL;

// If true, game is running as a screensaver

boolean screensaver_mode = false;

// Flag indicating whether the screen is currently visible:
// when the screen isnt visible, don't render the screen

boolean screenvisible;

//buttons mappings for camera
//Final array size must be a even number which isfollowed by a Unpress keyevent and a Press keyevent
// Unpress Press Unpress Press Unpress Press..
int buttonEvents[] = {BGMT_Q_SET_UNPRESS, BGMT_Q_SET, BGMT_UNPRESS_UP, BGMT_PRESS_UP, BGMT_UNPRESS_DOWN, BGMT_PRESS_DOWN, BGMT_UNPRESS_LEFT, BGMT_PRESS_LEFT, BGMT_UNPRESS_RIGHT, BGMT_PRESS_RIGHT};
int buttonMap[] = {KEY_FIRE, KEY_UPARROW, KEY_DOWNARROW, KEY_LEFTARROW, KEY_RIGHTARROW};

int pushbuttonEvents[] = {BGMT_PLAY, BGMT_MENU, BGMT_INFO};
int pushbuttonMap[] = {KEY_ENTER, KEY_ESCAPE, 'y'};

// Mouse acceleration
//
// This emulates some of the behavior of DOS mouse drivers by increasing
// the speed when the mouse is moved fast.
//
// The mouse input values are input directly to the game, but when
// the values exceed the value of mouse_threshold, they are multiplied
// by mouse_acceleration to increase the speed.

float mouse_acceleration = 2.0;
int mouse_threshold = 10;

// Gamma correction level to use

int usegamma = 0;

int usemouse = 0;
extern int *global_next_weapon;
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
unsigned int handle;
void I_InitGraphics(void)
{

    I_VideoBuffer = (byte *)Z_Malloc(SCREENWIDTH * SCREENHEIGHT, PU_STATIC, NULL);
    screenvisible = true;
}

void I_ShutdownGraphics(void)
{
    Z_Free(I_VideoBuffer);
}

void I_StartFrame(void)
{
}
extern inited;
struct gui_main_struct
{
    void *obj; // off_0x00;
    uint32_t counter_550d;
    uint32_t off_0x08;
    uint32_t counter; // off_0x0c;
    uint32_t off_0x10;
    uint32_t off_0x14;
    uint32_t off_0x18;
    uint32_t off_0x1c;
    uint32_t off_0x20;
    uint32_t off_0x24;
    uint32_t off_0x28;
    uint32_t off_0x2c;
    struct msg_queue *msg_queue;      // off_0x30;
    struct msg_queue *off_0x34;       // off_0x34;
    struct msg_queue *msg_queue_550d; // off_0x38;
    uint32_t off_0x3c;
};
extern struct gui_main_struct gui_main_struct;
void ml_gui_main_task()
{
    gui_init_end();
    struct event *event = NULL;
    event_t Doomevent;
    int index = 0;
    void *funcs[GMT_NFUNCS];
    memcpy(funcs, (void *)GMT_FUNCTABLE, 4 * GMT_NFUNCS);

    gui_init_end(); // no params?

    while (1)
    {
#if defined(CONFIG_550D) || defined(CONFIG_7D)
        msg_queue_receive(gui_main_struct.msg_queue_550d, &event, 0);
        gui_main_struct.counter_550d--;
#else
        msg_queue_receive(gui_main_struct.msg_queue, &event, 0);
        gui_main_struct.counter--;
#endif

        if (event == NULL)
        {
            continue;
        }

        index = event->type;
        if (inited)
        {

            //uart_printf("// ML button/event handler EVENT: 0x%x TYPE:0x%x\n", event->param, event->type);
            //ignore any GUI_Control options. is it save?
            if (event->type == 1)
            {
                //uart_printf("// ML button/event handler EVENT: 0x%x TYPE:0x%x\n", event->param, event->type);
                continue;
            }

            if (event->type == 0)
            {
                //uart_printf("// ML button/event handler EVENT: 0x%x TYPE:0x%x\n", event->param, event->type);
                for (int index = 0; index < sizeof(buttonEvents) / sizeof(int); index++)
                {
                    if (event->param == buttonEvents[index])
                    {
                        if (index % 2)
                        {
                            //uart_printf("Keydown! 0x%x DoomEvent:0x%x\n",buttonEvents[index-1],buttonMap[(index) >> 1]);
                            Doomevent.type = ev_keydown;
                            Doomevent.data1 = buttonMap[(index) >> 1];
                            Doomevent.data2 = -1;
                            Doomevent.data3 = -1;
                            D_PostEvent(&Doomevent);
                            continue;
                        }
                        else
                        {
                            // uart_printf("Keyup! 0x%x Index:0x%x\n",buttonEvents[index],(index) >> 1);
                            Doomevent.type = ev_keyup;
                            Doomevent.data1 = buttonMap[(index) >> 1];
                            Doomevent.data2 = -1;
                            Doomevent.data3 = -1;
                            D_PostEvent(&Doomevent);
                            continue;
                        }
                    }
                }
                for (int index = 0; index < sizeof(pushbuttonEvents) / sizeof(int); index++)
                {
                    if (event->param == pushbuttonEvents[index])
                    {
                        //uart_printf(" 0x%x Index:0x%x\n",pushbuttonEvents[index], pushbuttonMap[index]);
                        Doomevent.type = ev_keydown;
                        Doomevent.data1 = pushbuttonMap[index];
                        Doomevent.data2 = -1;
                        Doomevent.data3 = -1;
                        D_PostEvent(&Doomevent);
                        continue;
                    }
                }
                // special case for incrementing weapons
                if (event->param == BGMT_WHEEL)
                {
                    if (global_next_weapon)
                    {
                        if (screenvisible)
                        {
                            *global_next_weapon = 1;
                        }
                    }
                    continue;
                }
                // most buttons are mapped are in this range (0x30 to 0x62) so this if statement avoids them getting passed to the GUI
                if (event->param <= 0x30 || event->param >= 0x60)
                {

                    continue;
                }
            }
            else
            {
            }
        }
        if (IS_FAKE(event))
        {
            event->arg = 0; /* do not pass the "fake" flag to Canon code */
        }

        if (event->type == 0 && event->param < 0)
        {
            continue; /* do not pass internal ML events to Canon code */
        }

        if ((index >= GMT_NFUNCS) || (index < 0))
        {
            continue;
        }

        void (*f)(struct event *) = funcs[index];
        f(event);
    }
}
void I_GetEvent(void)
{
}

void I_StartTic(void)
{
    I_GetEvent();
}

void I_UpdateNoBlit(void)
{
}



//7% increese in speed with unrolling
//TODO: find a better way to refill the VRAM buffer
//send help now this is too bad
void I_FinishUpdate(void)
{
    struct MARV *rgb_MARV = MEM(0xfd80);
    uint32_t *b = rgb_MARV->bitmap_data;

    const int src_w = 320;
    const int src_h = 200;
    const int dst_x = 160;
    const int dst_y = 70;
    const int out_w = 960;
    int src_y = 0;

    //value to go from row+0 end to row+2 start
    const int row_ofsfet = out_w-src_w-src_w;

    //two pointers to draw 2 rows at once
    uint32_t *row1 = b + out_w*(dst_y) + dst_x;
    uint32_t *row2 = b + out_w*(dst_y+1) + dst_x;

    //to reduce lookups per loop
    uint32_t color = 0;
    for (int i = 0; i < src_h; i++)
    {
        src_y = i * src_w;
        for(int j = 0; j < src_w; j++){
            //compute color once
            color = rgb32_palette[I_VideoBuffer[src_y + j]];
            //draw twice in each row
            *row1++ = color;
            *row1++ = color;
            *row2++ = color;
            *row2++ = color;
          }
        //move each pointer by two rows
        row1 = row1 + out_w + row_ofsfet;
        row2 = row2 + out_w + row_ofsfet;
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
