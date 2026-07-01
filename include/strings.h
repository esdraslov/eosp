#ifndef STRINGS_H
#define STRINGS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void *memset(void *s, int c, int n);

void *memcpy(void *dest, const void *src, uint32_t n);

size_t strlen(const char* str);

void itoa(uint32_t n, char *buffer);

uint32_t atoi(const char *str);

bool strcmp(const char *s1, const char *s2);

int strcasecmp(const char *s1, const char *s2);

void strcpy(char *dest, const char *src);

int next_path_token(const char **path_ptr, char *token_buffer);

#endif
