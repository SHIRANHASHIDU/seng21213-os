#ifndef STRING_H
#define STRING_H

#include <stddef.h>

void  *memset(void *dst, int val, size_t len);
void  *memcpy(void *dst, const void *src, size_t len);
size_t strlen(const char *s);
int    strcmp(const char *a, const char *b);
char  *strcpy(char *dst, const char *src);
int    strncmp(const char *a, const char *b, size_t n);
char  *strtok_ws(char *str, char **saveptr);

#endif
