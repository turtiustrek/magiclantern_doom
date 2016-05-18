#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "eos.h"
#include "mpu.h"

// Forward declare static functions
static void mpu_send_next_spell(EOSState *s);
static void mpu_enqueue_spell(EOSState *s, int spell_set, int out_spell);
static void mpu_interpret_command(EOSState *s);


#define MPU_CURRENT_OUT_SPELL mpu_init_spells[s->mpu.spell_set].out_spells[s->mpu.out_spell]
/**
 * We don't know the meaning of MPU messages yet, so we'll replay them from a log file.
 * Timing is important - if we would just send everything as a response to first message,
 * the tasks that handle that message may not be started yet.
 * 
 * We will attempt to guess a causal relationship between mpu_send and mpu_recv calls.
 * Although not perfect, that guess is a good starting point.
 * 
 * These values are valid for a 60D.
 */

struct mpu_init_spell mpu_init_spells[] = { {
    { 0x06, 0x04, 0x02, 0x00, 0x00 }, {                         /* spell #1 */
        { 0x08, 0x07, 0x01, 0x33, 0x09, 0x00, 0x00, 0x00 },     /* reply #1.1 */
        { 0x06, 0x05, 0x01, 0x20, 0x00, 0x00 },                 /* reply #1.2 */
        { 0x06, 0x05, 0x01, 0x21, 0x01, 0x00 },                 /* reply #1.3 */
        { 0x06, 0x05, 0x01, 0x22, 0x00, 0x00 },                 /* reply #1.4 */
        { 0x06, 0x05, 0x03, 0x0c, 0x01, 0x00 },                 /* reply #1.5 */
        { 0x06, 0x05, 0x03, 0x0d, 0x01, 0x00 },                 /* reply #1.6 */
        { 0x06, 0x05, 0x03, 0x0e, 0x01, 0x00 },                 /* reply #1.7 */
        { 0x08, 0x06, 0x01, 0x23, 0x00, 0x01, 0x00 },           /* reply #1.8 */
        { 0x08, 0x06, 0x01, 0x24, 0x00, 0x00, 0x00 },           /* reply #1.9 */
        { 0x08, 0x06, 0x01, 0x25, 0x00, 0x01, 0x00 },           /* reply #1.10 */
        { 0x06, 0x05, 0x01, 0x2e, 0x01, 0x00 },                 /* reply #1.11 */
        { 0x06, 0x05, 0x01, 0x2c, 0x02, 0x00 },                 /* reply #1.12 */
        { 0x06, 0x05, 0x03, 0x20, 0x04, 0x00 },                 /* reply #1.13 */
        { 0x06, 0x05, 0x01, 0x3d, 0x00, 0x00 },                 /* reply #1.14 */
        { 0x06, 0x05, 0x01, 0x42, 0x00, 0x00 },                 /* reply #1.15 */
        { 0x06, 0x05, 0x01, 0x00, 0x03, 0x00 },                 /* reply #1.16 */
        { 0x2c, 0x2a, 0x02, 0x00, 0x03, 0x03, 0x03, 0x04, 0x03, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x14, 0x50, 0x00, 0x00, 0x00, 0x00, 0x81, 0x06, 0x00, 0x00, 0x04, 0x06, 0x00, 0x00, 0x04, 0x06, 0x00, 0x00, 0x04, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x4d, 0x4b, 0x01 },/* reply #1.17 */
        { 0x0c, 0x0b, 0x01, 0x0a, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },/* reply #1.18 */
        { 0x06, 0x05, 0x01, 0x37, 0x00, 0x00 },                 /* reply #1.19 */
        { 0x06, 0x05, 0x01, 0x49, 0x01, 0x00 },                 /* reply #1.20 */
        { 0x06, 0x05, 0x01, 0x3e, 0x00, 0x00 },                 /* reply #1.21 */
        { 0x08, 0x06, 0x01, 0x45, 0x00, 0x10, 0x00 },           /* reply #1.22 */
        { 0x06, 0x05, 0x01, 0x48, 0x01, 0x00 },                 /* reply #1.23 */
        { 0x06, 0x05, 0x01, 0x4b, 0x01, 0x00 },                 /* reply #1.24 */
        { 0x06, 0x05, 0x01, 0x40, 0x00, 0x00 },                 /* reply #1.25 */
        { 0x06, 0x05, 0x01, 0x41, 0x00, 0x00 },                 /* reply #1.26 */
        { 0x06, 0x05, 0x01, 0x3f, 0x00, 0x00 },                 /* reply #1.27 */
        { 0x1a, 0x18, 0x01, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00 },/* reply #1.28 */
        { 0x06, 0x05, 0x01, 0x48, 0x01, 0x00 },                 /* reply #1.29 */
        { 0x06, 0x05, 0x01, 0x53, 0x00, 0x00 },                 /* reply #1.30 */
        { 0x06, 0x05, 0x01, 0x4a, 0x00, 0x00 },                 /* reply #1.31 */
        { 0x06, 0x05, 0x01, 0x50, 0x03, 0x00 },                 /* reply #1.32 */
        { 0x08, 0x06, 0x01, 0x51, 0x70, 0x48, 0x00 },           /* reply #1.33 */
        { 0x06, 0x05, 0x01, 0x52, 0x00, 0x00 },                 /* reply #1.34 */
        { 0x06, 0x05, 0x01, 0x54, 0x00, 0x00 },                 /* reply #1.35 */
        { 0x06, 0x05, 0x03, 0x37, 0x00, 0x00 },                 /* reply #1.36 */
        { 0x0e, 0x0c, 0x02, 0x05, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 },/* reply #1.37 */
        { 0x0a, 0x08, 0x02, 0x06, 0x04, 0x00, 0x00, 0x00, 0x00 },/* reply #1.38 */
        { 0x0c, 0x0a, 0x02, 0x07, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },/* reply #1.39 */
        { 0x0c, 0x0a, 0x02, 0x08, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 },/* reply #1.40 */
        { 0x0a, 0x08, 0x03, 0x2f, 0x00, 0x00, 0x00, 0x00, 0x00 },/* reply #1.41 */
        { 0x06, 0x05, 0x03, 0x05, 0x02, 0x00 },                 /* reply #1.42 */
        { 0x1e, 0x1c, 0x03, 0x30, 0x65, 0x65, 0x50, 0x50, 0x53, 0x53, 0x53, 0x53, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00 },/* reply #1.43 */
        { 0x0e, 0x0c, 0x03, 0x2e, 0x00, 0x00, 0x83, 0xad, 0x00, 0x00, 0xdb, 0x71, 0x00 },/* reply #1.44 */
        { 0x06, 0x05, 0x03, 0x35, 0x01, 0x00 },                 /* reply #1.45 */
        { 0x1c, 0x1b, 0x03, 0x1d, 0x4a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4c, 0x50, 0x2d, 0x45, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xae, 0x7e, 0x3b, 0x61, 0x00 },/* reply #1.46 */
        { 0x06, 0x04, 0x03, 0x36, 0x00 },                       /* reply #1.47 */
        { 0 } } }, {
    { 0x08, 0x06, 0x00, 0x00, 0x02, 0x00, 0x00 }, {             /* spell #2 */
        { 0x08, 0x07, 0x01, 0x55, 0x00, 0x02, 0x01, 0x01 },     /* reply #2.1 */
        { 0 } } }, {
    { 0x06, 0x05, 0x01, 0x2e, 0x01, 0x00 }, {                   /* spell #3 */
        { 0x06, 0x05, 0x01, 0x2e, 0x01, 0x00 },                 /* reply #3.1 */
        { 0 } } }, {
    { 0x06, 0x05, 0x03, 0x40, 0x00, 0x00 }, {                   /* spell #4 */
        { 0x06, 0x05, 0x03, 0x38, 0x95, 0x00 },                 /* reply #4.1 */
        { 0 } } }, {
    { 0x08, 0x06, 0x01, 0x24, 0x00, 0x01, 0x00 }, {             /* spell #5 */
        { 0x08, 0x06, 0x01, 0x24, 0x00, 0x01, 0x00 },           /* reply #5.1 */
        { 0 } } }, {
    { 0x06, 0x05, 0x03, 0x0c, 0x00, 0x00 }, {                   /* spell #6 */
        { 0x06, 0x05, 0x01, 0x2c, 0x02, 0x00 },                 /* reply #6.1 */
        { 0x0a, 0x08, 0x03, 0x00, 0x6c, 0x00, 0x00, 0x2f, 0x00 },/* reply #6.2 */
        { 0x06, 0x05, 0x03, 0x04, 0x00, 0x00 },                 /* reply #6.3 */
        { 0x1a, 0x18, 0x03, 0x15, 0x01, 0x2d, 0x58, 0x00, 0x30, 0x00, 0x12, 0x00, 0x37, 0x91, 0x75, 0x92, 0x1f, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00 },/* reply #6.4 */
        { 0x24, 0x22, 0x03, 0x3c, 0x00, 0x00, 0x88, 0xb5, 0x7b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },/* reply #6.5 */
        { 0x06, 0x05, 0x03, 0x17, 0x92, 0x00 },                 /* reply #6.6 */
        { 0x06, 0x05, 0x03, 0x23, 0x19, 0x00 },                 /* reply #6.7 */
        { 0x1e, 0x1d, 0x03, 0x24, 0x45, 0x46, 0x2d, 0x53, 0x31, 0x38, 0x2d, 0x35, 0x35, 0x6d, 0x6d, 0x20, 0x66, 0x2f, 0x33, 0x2e, 0x35, 0x2d, 0x35, 0x2e, 0x36, 0x20, 0x49, 0x53, 0x00, 0x00 },/* reply #6.8 */
        { 0x06, 0x04, 0x03, 0x25, 0x00 },                       /* reply #6.9 */
        { 0x06, 0x05, 0x01, 0x3d, 0x00, 0x00 },                 /* reply #6.10 */
        { 0x06, 0x05, 0x03, 0x37, 0x00, 0x00 },                 /* reply #6.11 */
        { 0x06, 0x05, 0x03, 0x0d, 0x00, 0x00 },                 /* reply #6.12 */
        { 0x1a, 0x18, 0x03, 0x15, 0x01, 0x2d, 0x58, 0x00, 0x30, 0x00, 0x12, 0x00, 0x37, 0x91, 0x75, 0x92, 0x1f, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00 },/* reply #6.13 */
        { 0x06, 0x05, 0x03, 0x0c, 0x00, 0x00 },                 /* reply #6.14 */
        { 0 } } }, {
    { 0x1a, 0x18, 0x01, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00 }, {/* spell #7 */
        { 0x1a, 0x18, 0x01, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00 },/* reply #7.1 */
        { 0x06, 0x05, 0x01, 0x53, 0x00, 0x00 },                 /* reply #7.2 */
        { 0 } } }, {
    { 0x1a, 0x18, 0x01, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00 }, {/* spell #8 */
        { 0x1a, 0x18, 0x01, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00 },/* reply #8.1 */
        { 0x06, 0x05, 0x01, 0x53, 0x00, 0x00 },                 /* reply #8.2 */
        { 0 } } }, {
    { 0x06, 0x05, 0x02, 0x0a, 0x01, 0x00 }, {                   /* spell #9 */
        { 0x06, 0x05, 0x06, 0x11, 0x01, 0x00 },                 /* reply #9.1 */
        { 0x06, 0x05, 0x06, 0x12, 0x00, 0x00 },                 /* reply #9.2 */
        { 0x06, 0x05, 0x06, 0x13, 0x00, 0x00 },                 /* reply #9.3 */
        { 0x42, 0x41, 0x0a, 0x08, 0xff, 0x1f, 0x01, 0x00, 0x01, 0x01, 0xa0, 0x10, 0x00, 0x4d, 0x01, 0x01, 0x58, 0x2d, 0x4b, 0x01, 0x01, 0x00, 0x48, 0x04, 0x01, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },/* reply #9.4 */
        { 0x06, 0x05, 0x04, 0x0e, 0x01, 0x00 },                 /* reply #9.5 */
        { 0 } } }, {
    { 0x06, 0x05, 0x03, 0x1d, 0x1f, 0x00 }, {                   /* spell #10 */
        { 0x06, 0x05, 0x03, 0x35, 0x01, 0x00 },                 /* reply #10.1 */
        { 0x1c, 0x1b, 0x03, 0x1d, 0x4a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4c, 0x50, 0x2d, 0x45, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xae, 0x7e, 0x3b, 0x61, 0x00 },/* reply #10.2 */
        { 0x06, 0x04, 0x03, 0x36, 0x00 },                       /* reply #10.3 */
        { 0 } } }, {
    { 0x06, 0x05, 0x08, 0x06, 0xff, 0x00 }, {                   /* spell #11 */
        { 0x06, 0x05, 0x08, 0x06, 0x00, 0x00 },                 /* reply #11.1 */
    { 0 } } }
};

/**
 * Alternative version: send everything after the first message,
 * with one exception: delay GUI-related messages.
 */
struct mpu_init_spell mpu_init_spells_alt[] = { {
    { 0x06, 0x04, 0x02, 0x00, 0x00 }, {
        { 0x08, 0x07, 0x01, 0x33, 0x09, 0x00, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x20, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x21, 0x01, 0x00 },
        { 0x06, 0x05, 0x01, 0x22, 0x00, 0x00 },
        { 0x06, 0x05, 0x03, 0x0c, 0x01, 0x00 },
        { 0x06, 0x05, 0x03, 0x0d, 0x01, 0x00 },
        { 0x06, 0x05, 0x03, 0x0e, 0x01, 0x00 },
        { 0x08, 0x06, 0x01, 0x23, 0x00, 0x01, 0x00 },
        { 0x08, 0x06, 0x01, 0x24, 0x00, 0x00, 0x00 },
        { 0x08, 0x06, 0x01, 0x25, 0x00, 0x01, 0x00 },
        { 0x06, 0x05, 0x01, 0x2e, 0x01, 0x00 },
        { 0x06, 0x05, 0x01, 0x2c, 0x02, 0x00 },
        { 0x06, 0x05, 0x03, 0x20, 0x04, 0x00 },
        { 0x06, 0x05, 0x01, 0x3d, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x42, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x00, 0x03, 0x00 },
        { 0x2c, 0x2a, 0x02, 0x00, 0x03, 0x03, 0x03, 0x04, 0x03, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x14, 0x50, 0x00, 0x00, 0x00, 0x00, 0x81, 0x06, 0x00, 0x00, 0x04, 0x06, 0x00, 0x00, 0x04, 0x06, 0x00, 0x00, 0x04, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x4d, 0x4b, 0x01 },
        { 0x0c, 0x0b, 0x01, 0x0a, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x37, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x49, 0x01, 0x00 },
        { 0x06, 0x05, 0x01, 0x3e, 0x00, 0x00 },
        { 0x08, 0x06, 0x01, 0x45, 0x00, 0x10, 0x00 },
        { 0x06, 0x05, 0x01, 0x48, 0x01, 0x00 },
        { 0x06, 0x05, 0x01, 0x4b, 0x01, 0x00 },
        { 0x06, 0x05, 0x01, 0x40, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x41, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x3f, 0x00, 0x00 },
        { 0x1a, 0x18, 0x01, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x48, 0x01, 0x00 },
        { 0x06, 0x05, 0x01, 0x53, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x4a, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x50, 0x03, 0x00 },
        { 0x08, 0x06, 0x01, 0x51, 0x70, 0x48, 0x00 },
        { 0x06, 0x05, 0x01, 0x52, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x54, 0x00, 0x00 },
        { 0x06, 0x05, 0x03, 0x37, 0x00, 0x00 },
        { 0x0e, 0x0c, 0x02, 0x05, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x0a, 0x08, 0x02, 0x06, 0x04, 0x00, 0x00, 0x00, 0x00 },
        { 0x0c, 0x0a, 0x02, 0x07, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x0c, 0x0a, 0x02, 0x08, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x0a, 0x08, 0x03, 0x2f, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x06, 0x05, 0x03, 0x05, 0x02, 0x00 },
        { 0x1e, 0x1c, 0x03, 0x30, 0x65, 0x65, 0x50, 0x50, 0x53, 0x53, 0x53, 0x53, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00 },
        { 0x0e, 0x0c, 0x03, 0x2e, 0x00, 0x00, 0x83, 0xad, 0x00, 0x00, 0xdb, 0x71, 0x00 },
        { 0x06, 0x05, 0x03, 0x35, 0x01, 0x00 },
        { 0x1c, 0x1b, 0x03, 0x1d, 0x4a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4c, 0x50, 0x2d, 0x45, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xae, 0x7e, 0x3b, 0x61, 0x00 },
        { 0x06, 0x04, 0x03, 0x36, 0x00 },
        { 0x08, 0x07, 0x01, 0x55, 0x00, 0x02, 0x01, 0x01 },
        { 0x06, 0x05, 0x01, 0x2e, 0x01, 0x00 },
        { 0x06, 0x05, 0x03, 0x38, 0x95, 0x00 },
        { 0x08, 0x06, 0x01, 0x24, 0x00, 0x01, 0x00 },
        { 0x06, 0x05, 0x01, 0x2c, 0x02, 0x00 },
        { 0x0a, 0x08, 0x03, 0x00, 0x6c, 0x00, 0x00, 0x2f, 0x00 },
        { 0x06, 0x05, 0x03, 0x04, 0x00, 0x00 },
        { 0x1a, 0x18, 0x03, 0x15, 0x01, 0x2d, 0x58, 0x00, 0x30, 0x00, 0x12, 0x00, 0x37, 0x91, 0x75, 0x92, 0x1f, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00 },
        { 0x24, 0x22, 0x03, 0x3c, 0x00, 0x00, 0x88, 0xb5, 0x7b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x06, 0x05, 0x03, 0x17, 0x92, 0x00 },
        { 0x06, 0x05, 0x03, 0x23, 0x19, 0x00 },
        { 0x1e, 0x1d, 0x03, 0x24, 0x45, 0x46, 0x2d, 0x53, 0x31, 0x38, 0x2d, 0x35, 0x35, 0x6d, 0x6d, 0x20, 0x66, 0x2f, 0x33, 0x2e, 0x35, 0x2d, 0x35, 0x2e, 0x36, 0x20, 0x49, 0x53, 0x00, 0x00 },
        { 0x06, 0x04, 0x03, 0x25, 0x00 },
        { 0x06, 0x05, 0x01, 0x3d, 0x00, 0x00 },
        { 0x06, 0x05, 0x03, 0x37, 0x00, 0x00 },
        { 0x06, 0x05, 0x03, 0x0d, 0x00, 0x00 },
        { 0x1a, 0x18, 0x03, 0x15, 0x01, 0x2d, 0x58, 0x00, 0x30, 0x00, 0x12, 0x00, 0x37, 0x91, 0x75, 0x92, 0x1f, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00 },
        { 0x06, 0x05, 0x03, 0x0c, 0x00, 0x00 },
        { 0x1a, 0x18, 0x01, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x53, 0x00, 0x00 },
        { 0x1a, 0x18, 0x01, 0x4e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x06, 0x05, 0x01, 0x53, 0x00, 0x00 },
        { 0 } } }, {
    { 0x06, 0x05, 0x02, 0x0a, 0x01, 0x00 }, {       /* wait for this message before continuing */
        { 0x06, 0x05, 0x06, 0x11, 0x01, 0x00 },     /* although not correct (it's probably sensor cleaning related), this trick appears to launch the GUI */
        { 0x06, 0x05, 0x06, 0x12, 0x00, 0x00 },
        { 0x06, 0x05, 0x06, 0x13, 0x00, 0x00 },
        { 0x42, 0x41, 0x0a, 0x08, 0xff, 0x1f, 0x01, 0x00, 0x01, 0x01, 0xa0, 0x10, 0x00, 0x4d, 0x01, 0x01, 0x58, 0x2d, 0x4b, 0x01, 0x01, 0x00, 0x48, 0x04, 0x01, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x06, 0x05, 0x04, 0x0e, 0x01, 0x00 },
        { 0x06, 0x05, 0x03, 0x35, 0x01, 0x00 },
        { 0x1c, 0x1b, 0x03, 0x1d, 0x4a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4c, 0x50, 0x2d, 0x45, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xae, 0x7e, 0x3b, 0x61, 0x00 },
        { 0x06, 0x04, 0x03, 0x36, 0x00 },
        { 0x06, 0x05, 0x08, 0x06, 0x00, 0x00 },
    { 0 } } }
};

static void mpu_send_next_spell(EOSState *s)
{
    if (s->mpu.sq_head != s->mpu.sq_tail)
    {
        /* get next spell from the queue */
        s->mpu.spell_set = s->mpu.send_queue[s->mpu.sq_head].spell_set;
        s->mpu.out_spell = s->mpu.send_queue[s->mpu.sq_head].out_spell;
        s->mpu.sq_head = (s->mpu.sq_head+1) & (COUNT(s->mpu.send_queue)-1);
        printf("[MPU] Sending spell #%d.%d ( ", s->mpu.spell_set+1, s->mpu.out_spell+1);

        int i;
        for (i = 0; i < MPU_CURRENT_OUT_SPELL[0]; i++)
        {
            printf("%02x ", MPU_CURRENT_OUT_SPELL[i]);
        }
        printf(")\n");

        s->mpu.out_char = -2;

        /* request a SIO3 interrupt */
        eos_trigger_int(s, 0x36, 0);
    }
    else
    {
        printf("[MPU] Nothing more to send.\n");
        s->mpu.sending = 0;
    }
}

static void mpu_enqueue_spell(EOSState *s, int spell_set, int out_spell)
{
    int next_tail = (s->mpu.sq_tail+1) & (COUNT(s->mpu.send_queue)-1);
    if (next_tail != s->mpu.sq_head)
    {
        printf("[MPU] Queueing spell #%d.%d\n", spell_set+1, out_spell+1);
        s->mpu.send_queue[s->mpu.sq_tail].spell_set = spell_set;
        s->mpu.send_queue[s->mpu.sq_tail].out_spell = out_spell;
        s->mpu.sq_tail = next_tail;
    }
    else
    {
        printf("[MPU] ERROR: send queue full\n");
    }
}


static void mpu_interpret_command(EOSState *s)
{
    printf("[MPU] Received: ");
    int i;
    for (i = 0; i < s->mpu.recv_index; i++)
    {
        printf("%02x ", s->mpu.recv_buffer[i]);
    }
    
    int spell_set;
    for (spell_set = 0; spell_set < COUNT(mpu_init_spells); spell_set++)
    {
        if (memcmp(s->mpu.recv_buffer+1, mpu_init_spells[spell_set].in_spell+1, s->mpu.recv_buffer[1]) == 0)
        {
            printf(" (recognized spell #%d)\n", spell_set+1);
            
            int out_spell;
            for (out_spell = 0; mpu_init_spells[spell_set].out_spells[out_spell][0]; out_spell++)
            {
                mpu_enqueue_spell(s, spell_set, out_spell);
            }
            
            if (!s->mpu.sending)
            {
                s->mpu.sending = 1;
                
                /* request a MREQ interrupt */
                eos_trigger_int(s, 0x50, 0);
            }
            return;
        }
    }
    
    printf(" (unknown spell)\n");
}

void mpu_handle_sio3_interrupt(EOSState *s)
{
    if (s->mpu.sending)
    {
        int num_chars = MPU_CURRENT_OUT_SPELL[0];
        
        if (num_chars)
        {
            /* next two num_chars */
            s->mpu.out_char += 2;
            
            if (s->mpu.out_char < num_chars)
            {
                /*
                printf(
                    "[MPU] Sending spell #%d.%d, chars %d & %d out of %d\n", 
                    s->mpu.spell_set+1, s->mpu.out_spell+1,
                    s->mpu.out_char+1, s->mpu.out_char+2,
                    num_chars
                );
                */
                
                if (s->mpu.out_char + 2 < num_chars)
                {
                    eos_trigger_int(s, 0x36, 0);   /* SIO3 */
                }
                else
                {
                    printf("[MPU] spell #%d.%d finished\n", s->mpu.spell_set+1, s->mpu.out_spell+1);

                    if (s->mpu.sq_head != s->mpu.sq_tail)
                    {
                        printf("[MPU] Requesting next spell\n");
                        eos_trigger_int(s, 0x50, 0);   /* MREQ */
                    }
                    else
                    {
                        /* no more spells */
                        printf("[MPU] spells finished\n");
                        s->mpu.sending = 0;
                    }
                }
            }
        }
    }

    if (s->mpu.receiving)
    {
        if (s->mpu.recv_index < s->mpu.recv_buffer[0])
        {
            /* more data to receive */
            printf("[MPU] Request more data\n");
            eos_trigger_int(s, 0x36, 0);   /* SIO3 */
        }
    }
}

void mpu_handle_mreq_interrupt(EOSState *s)
{
    if (s->mpu.sending)
    {
        mpu_send_next_spell(s);
    }
    
    if (s->mpu.receiving)
    {
        if (s->mpu.recv_index == 0)
        {
            printf("[MPU] receiving next message\n");
        }
        else
        {
            /* if a message is started in SIO3, it should continue with SIO3's, without triggering another MREQ */
            /* it appears to be harmless,  but I'm not sure what happens with more than 1 message queued */
            printf("[MPU] next message was started in SIO3\n");
        }
        eos_trigger_int(s, 0x36, 0);   /* SIO3 */
    }
}

unsigned int eos_handle_mpu(unsigned int parm, EOSState *s, unsigned int address, unsigned char type, unsigned int value )
{
    /* C022009C - MPU request/status
     * - set to 0x46 at startup
     * - bic 0x2 in mpu_send
     * - orr 0x2 at the end of a message sent to the MPU (SIO3_ISR, get_data_to_send)
     * - tst 0x2 in SIO3 and MREQ ISRs
     * - should return 0x44 when sending data to MPU
     * - and 0x47 when receiving data from MPU
     */
    
    int ret = 0;
    const char * msg = 0;
    intptr_t msg_arg1 = 0;
    intptr_t msg_arg2 = 0;
    int receive_finished = 0;

    if(type & MODE_WRITE)
    {
        s->mpu.status = value;
        
        if (value & 2)
        {
            if (s->mpu.receiving)
            {
                if (s->mpu.recv_index == s->mpu.recv_buffer[0])
                {
                    msg = "Receive finished";
                    s->mpu.receiving = 0;
                    receive_finished = 1;
                }
                else
                {
                    msg = "Unknown request while receiving";
                }
            }
            else if (s->mpu.sending)
            {
                msg = "Unknown request while sending";
            }
            else /* idle */
            {
                if (value == 0x46)
                {
                    msg = "init";
                }
            }
        }
        else
        {
            msg = "Receive request %s";
            msg_arg1 = (intptr_t) "";
            
            if (s->mpu.receiving)
            {
                msg_arg1 = (intptr_t) "(I'm busy receiving stuff!)";
            }
            else if (s->mpu.sending)
            {
                msg_arg1 = (intptr_t) "(I'm busy sending stuff, but I'll try!)";
                s->mpu.receiving = 1;
                s->mpu.recv_index = 0;
            }
            else
            {
                s->mpu.receiving = 1;
                s->mpu.recv_index = 0;
                eos_trigger_int(s, 0x50, 0);   /* MREQ */
                /* next steps in eos_handle_mreq -> mpu_handle_mreq_interrupt */
            }
        }
    }
    else
    {
        ret = (s->mpu.sending && !s->mpu.receiving) ? 0x3 :  /* I have data to send */
              (!s->mpu.sending && s->mpu.receiving) ? 0x0 :  /* I'm ready to receive data */
              (s->mpu.sending && s->mpu.receiving)  ? 0x1 :  /* I'm ready to send and receive data */
                                                      0x2 ;  /* I believe this is some error code */
        ret |= (s->mpu.status & 0xFFFFFFFC);                 /* The other bits are unknown;
                                                                they are set to 0x44 by writing to the register */

        msg = "status (sending=%d, receiving=%d)";
        msg_arg1 = s->mpu.sending;
        msg_arg2 = s->mpu.receiving;
    }

    io_log("MPU", s, address, type, value, ret, msg, msg_arg1, msg_arg2);

    if (receive_finished)
    {
        mpu_interpret_command(s);
    }
    
    return ret;
}

int mpu_handle_get_data(EOSState *s, int *hi, int *lo)
{
    if (s->mpu.spell_set < COUNT(mpu_init_spells) &&
        s->mpu.out_spell >= 0 &&
        s->mpu.out_char >= 0 && s->mpu.out_char < MPU_CURRENT_OUT_SPELL[0])
    {
        *hi = MPU_CURRENT_OUT_SPELL[s->mpu.out_char];
        *lo = MPU_CURRENT_OUT_SPELL[s->mpu.out_char+1];
		return 1;
    }
	return 0;
}


