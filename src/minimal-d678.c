/** \file
 * Minimal test code for DIGIC 6
 * ROM dumper & other experiments
 */

#include "dryos.h"
#include "bmp.h"
#include "log-d678.h"
#include "extfunctions.h"

extern void InitializeMzrmZtoMSnd();
extern void EvShell(int a);

extern void dump_file(char *name, uint32_t addr, uint32_t size);
extern void malloc_info(void);
extern void sysmem_info(void);
extern void smemShowFix(void);
extern void font_draw(uint32_t, uint32_t, uint32_t, uint32_t, char *);
extern void gui_main_task();
static uint32_t disp_xres = 0;
static uint8_t *disp_framebuf = NULL;
static char *vram_next = NULL;
static char *vram_current = NULL;
extern void D_DoomMain(void);
static int inited = 0;

static void DUMP_ASM task_doom()
{

    while (!bmp_vram_raw())
    {
        msleep(100);
    }
    uart_printf("Starting doom!");
    D_DoomMain();
    while (true)
    {
        uart_printf("Hello! I am in a error state so um yeah\n");
        msleep(1000);
    }
}
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
extern int button_event;
extern bool button_state;
void ml_gui_main_task()
{
    gui_init_end();
    struct event *event = NULL;
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
                continue;
            }
            if (event->type == 0)
            {
                uart_printf("// ML button/event handler EVENT: 0x%x TYPE:0x%x\n", event->param, event->type);

                switch (event->param)
                {

                case BGMT_Q_SET:
                case BGMT_PRESS_UP:
                case BGMT_PRESS_LEFT:
                case BGMT_PRESS_RIGHT:
                case BGMT_PRESS_DOWN:
                    button_event = event->param;
                    button_state = 1;
                    continue;
                }
                //since most buttons are mapped are in this range (0x30 to 0x62) so this if statement avoids them getting passed to the GUI
                if (event->param <= 0x30 || event->param >= 0x60)
                {
                    button_state = 0;
                    continue;
                }
            }
            else
            {
                button_state = 0;
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
static void
my_task_dispatch_hook(
    struct context **p_context_old, /* on new DryOS (6D+), this argument is different (small number, unknown meaning) */
    struct task *prev_task_unused,  /* only present on new DryOS */
    struct task *next_task_new      /* only present on new DryOS; old versions use HIJACK_TASK_ADDR */
)
{
    struct task *next_task = next_task_new;
    if (!next_task)
        return;

    struct context *context = next_task->context;
    if (!context)
        return;
    // Do nothing unless a new task is starting via the trampoile
    if (context->pc != (uint32_t)task_trampoline)
        return;

    thunk entry = (thunk)next_task->entry;

    if (entry == &gui_main_task)
    { //gui_main_task entry
        next_task->entry = &ml_gui_main_task;
    }
}
/* called before Canon's init_task */
void boot_pre_init_task(void)
{
    task_dispatch_hook = &my_task_dispatch_hook;
}

/* called right after Canon's init_task, while their initialization continues in background */
void boot_post_init_task(void)
{
    //MEM(0x115d8) = (int)&uart_printf | 1;
    //MEM(0x115e4) =  (int)&uart_printf | 1;
    msleep(1000);
    inited = 1;
    task_create("DOOM", 0x1f, 0x1000, task_doom, 0);
}

/* used by font_draw */
/* we don't have a valid display buffer yet */
void disp_set_pixel(int x, int y, int c)
{
}

#ifndef CONFIG_5D4
/* dummy */

#endif

void ml_assert_handler(char *msg, char *file, int line, const char *func){};
