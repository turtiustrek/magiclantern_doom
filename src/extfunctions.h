
#ifndef _extfunctions_h_
#define _extfunctions_h_




extern void * _AllocateMemory(size_t);
extern void _FreeMemory(void *);

extern void uart_printf(const char*,...);

#endif