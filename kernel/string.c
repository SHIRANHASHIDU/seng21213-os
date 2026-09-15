#include "string.h"

void *memset(void *dst, int val, size_t len) {
    unsigned char *p = (unsigned char *)dst;
    while (len--) {
        *p++ = (unsigned char)val;
    }
    return dst;
}

void *memcpy(void *dst, const void *src, size_t len) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (len--) {
        *d++ = *s++;
    }
    return dst;
}

size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

int strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
    while (n && *a && (*a == *b)) {
        a++;
        b++;
        n--;
    }
    if (n == 0) return 0;
    return (unsigned char)*a - (unsigned char)*b;
}

char *strcpy(char *dst, const char *src) {
    char *ret = dst;
    while ((*dst++ = *src++));
    return ret;
}

/* Simple whitespace tokenizer (no libc strtok available). */
char *strtok_ws(char *str, char **saveptr) {
    char *s = str ? str : *saveptr;
    if (!s) return NULL;

    while (*s == ' ' || *s == '\t') s++;
    if (*s == '\0') {
        *saveptr = NULL;
        return NULL;
    }

    char *token = s;
    while (*s && *s != ' ' && *s != '\t') s++;

    if (*s) {
        *s = '\0';
        *saveptr = s + 1;
    } else {
        *saveptr = NULL;
    }
    return token;
}
