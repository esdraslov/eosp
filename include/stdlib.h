// So I moved a lot of methods to here just so it's a smaller mess on the kernel.c file

#ifndef STDLIB_H
#define STDLIB_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include "strings.h"
#include "vga.h"

void printf(const char* format, ...);

#endif