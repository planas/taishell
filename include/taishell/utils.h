#ifndef __TAISHELL_UTILS_H__
#define __TAISHELL_UTILS_H__

#include <stdbool.h>

#ifdef __VITA_KERNEL__
#include <psp2kern/kernel/sysmem.h>
#else
#include <psp2/kernel/sysmem.h>
#endif

#define ALIGN(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

void * tshMemAlloc(SceKernelMemBlockType type, int size);
void tshMemFree(void *ptr);

void * ktshMemAlloc(SceKernelMemBlockType type, int size);
void ktshMemFree(void *ptr);

bool tshIsShell();
void tshSetButtonIntercept(bool state);
void tshDisableAutoSuspend();
void tshEnableAutoSuspend();

#endif