#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

typedef union MyFloat {
    float f;
    uint32_t i;
} MyFloat;

#if 0
/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.
 */
static char* itoa(int value, char* result, int base) {
    char *ptr = NULL;
    char *ptr1 = NULL;
    char tmp_char;
    int tmp_value;
    const char * const tbl = \
    "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz";

    /* check that the base if valid */
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    ptr = result;
    ptr1 = result;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = tbl[35 + (tmp_value - value * base)];
    } while (value);

    /* Apply negative sign */
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while(ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}

static void print_binary(uint32_t n) {
    char buffer [33];
    itoa(n, buffer, 2);
    printf("%s\n", buffer);
}
#endif

int main(void) {
    FILE *fp = NULL;
    MyFloat f;
    uint32_t i;

    fp = fopen("pow-lookup-table.bin", "wb");
    f.i = 0;

    for (i = 0; i < 1024; i++) {
        float ans;
        const uint32_t shift = 21;
        const uint32_t mask = (i << shift);

        f.i = 0;

        f.i |= mask;
        /*
        printf("f.i:  ");
        print_binary(f.i);
        printf("mask: ");
        print_binary(mask);
        */

        ans = log(f.f);
        fwrite(&ans, sizeof(ans), 1, fp);
        /*printf("%u: log(%f) = %f\n", i, f.f, ans);*/
        printf("%f,\n", ans);
    }

    fclose(fp);
    return 0;
}
