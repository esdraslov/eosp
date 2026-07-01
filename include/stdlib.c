#include "stdlib.h"

void printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    char buffer[256];
    int pos = 0;

    // (I don't know what this checks for)
    //               v I would guess this checks if what is here is the null terminator
    for (int i = 0; format[i]; i++)
    {
        if (format[i] == '%' && format[++i])
        {
            switch (format[i])
            {
                case 'd': {
                    int num = va_arg(args, int);
                    char buf[16];
                    itoa(num, buf);

                    for (int j = 0; buf[j]; j++)
                    {
                        buffer[pos++] = buf[j];
                    }
                    break;
                }
                case 's': {
                    const char *str = va_arg(args, const char*);
                    for (int j = 0; str[j]; j++)
                    {
                        buffer[pos++] = str[j];
                    }
                    break;
                }
				case 'c': {
					char c = va_arg(args, char);
					buffer[pos++] = c;
					break;
				}
                case '%': {
                    buffer[pos++] = '%';
                }
            }
        }
        else
        {
            buffer[pos++] = format[i];
        }
    }
    buffer[pos] = '\0';

    va_end(args);
    terminal_writestring(buffer);
}
