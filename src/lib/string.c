#include "string.h"

int strcmp(const char* s1, const char* s2) {
    while(*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while(n-- && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    if (n == (size_t)-1) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

size_t strlen(const char* s) {
    size_t len = 0;
    while(s[len]) len++;
    return len;
}

char* strcpy(char* dest, const char* src) {
    char* ret = dest;
    while((*dest++ = *src++));
    return ret;
}

char* strncpy(char* dest, const char* src, size_t n) {
    char* ret = dest;
    while(n && (*dest++ = *src++)) n--;
    while(n--) *dest++ = 0;
    return ret;
}
