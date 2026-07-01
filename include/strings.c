#include "strings.h"

void *memset(void *s, int c, int n)
{
	unsigned char *p = s;
	while (n--)
	{
		*p++ = (unsigned char)c;
	}
	return s;
}

void *memcpy(void *dest, const void *src, uint32_t n)
{
    // Cast the void pointers to byte pointers so we can increment them
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    // Copy byte by byte
    for (uint32_t i = 0; i < n; i++) {
        d[i] = s[i];
    }

    // Standard memcpy always returns the original destination pointer
    return dest;
}

size_t strlen(const char* str) 
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

void itoa(uint32_t n, char *buffer) // number to string
{
	char temp[16];
	int i = 0 ;

	if (n == 0)
	{
		buffer[0] = '0';
		buffer[1] = '\0';
		return;
	}

	while (n > 0)
	{
		temp[i++] = '0' + (n%10); 
		n /= 10;
	}

	int j = 0;
	while (i > 0) // put things in reverse is normal ig
	{
		buffer[j++] = temp[--i];
	}
	buffer[j] = '\0';
}

uint32_t atoi(const char *str) {
    uint32_t res = 0;
    for (int i = 0; str[i] != '\0'; ++i) {
        // Make sure we only parse actual digits
        if (str[i] >= '0' && str[i] <= '9') {
            res = res * 10 + str[i] - '0';
        } else {
            break; // Stop if a non-numeric character is found
        }
    }
    return res;
}

bool strcmp(const char *s1, const char *s2)
{
    while (*s1 && *s1 == *s2)
    {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

int strcasecmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        char c1 = (*s1 >= 'A' && *s1 <= 'Z') ? *s1 + 32 : *s1;
        char c2 = (*s2 >= 'A' && *s2 <= 'Z') ? *s2 + 32 : *s2;
        
        if (c1 != c2) {
            return c1 - c2;
        }
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

void strcpy(char *dest, const char *src)
{
    while (*src)
    {
        *dest = *src;
        dest++;
        src++;
    }
}

int next_path_token(const char **path_ptr, char *token_buffer) {
    const char *src = *path_ptr;
    int length = 0;

    // 1. Skip over any leading slashes (e.g., "///docs/notes.txt" -> "docs/notes.txt")
    while (*src == '/') {
        src++;
    }

    // 2. If we hit the end of the string, there are no more tokens left
    if (*src == '\0') {
        return 0;
    }

    // 3. Copy characters into our token buffer until we hit another slash or the end
    while (*src != '\0' && *src != '/') {
        // Prevent buffer overflows (assume token_buffer is at least 12 bytes for 8.3 FAT names)
        if (length < 255) { 
            token_buffer[length++] = *src;
        }
        src++;
    }

    // 4. Null-terminate our newly extracted token
    token_buffer[length] = '\0';

    // 5. Update the original path pointer so the next call picks up right where we left off!
    *path_ptr = src;

    return 1;
}
