#ifndef COMPAT_H
#define COMPAT_H

#include <psp2/kernel/clib.h>
#include <psp2/kernel/sysmem.h>

#include "taishell/utils.h"

int sscanf(const char *s, const char *format, ...);

#define snprintf  sceClibSnprintf
#define vsnprintf sceClibVsnprintf
#define strnlen   sceClibStrnlen
#define strncat   sceClibStrncat
#define strncpy   sceClibStrncpy
#define strrchr   sceClibStrrchr
#define strcmp    sceClibStrcmp
#define memset    sceClibMemset
#define memcpy    sceClibMemcpy
#define malloc(s) tshMemAlloc(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, ALIGN(s, 0xfff));
#define free(ptr) tshMemFree(ptr)

#endif