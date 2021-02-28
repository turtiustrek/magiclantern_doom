/** \file
 * Minimal test code for DIGIC 6
 * ROM dumper & other experiments
 */

#include "dryos.h"
#include "bmp.h"
#include "log-d678.h"
#include "extfunctions.h"
extern void dump_file(char* name, uint32_t addr, uint32_t size);
extern void malloc_info(void);
extern void sysmem_info(void);
extern void smemShowFix(void);
extern void font_draw(uint32_t, uint32_t, uint32_t, uint32_t, char*);

static uint32_t disp_xres = 0;
static uint8_t *disp_framebuf = NULL;
static char *vram_next = NULL;
static char *vram_current = NULL;
extern void D_DoomMain (void);




static void DUMP_ASM dump_task()
{
     while (!bmp_vram_raw())
    {
        msleep(100);
    }
    msleep(10000);
    uart_printf("Starting doom!");
    D_DoomMain ();
while (true)
{
uart_printf("Hello! I am in a error state so um yeah\n");
msleep(1000);
}

}

/* called before Canon's init_task */
void boot_pre_init_task(void)
{

}

/* called right after Canon's init_task, while their initialization continues in background */
void boot_post_init_task(void)
{

    msleep(1000);

    task_create("dump", 0x1e, 0x1000, dump_task, 0 );
}

/* used by font_draw */
/* we don't have a valid display buffer yet */
void disp_set_pixel(int x, int y, int c)
{
}

#ifndef CONFIG_5D4
/* dummy */

#endif

void ml_assert_handler(char* msg, char* file, int line, const char* func) { };
