#include <stdio.h>
#include <stdint.h>
#include <math.h>

float truncate_precision(float x) {
    uint32_t i;
    i = *(uint32_t *)&x;
    i &= ~0xffff;
    x = *(float *)&i;
    return x;
}

int main(void) {
    float f = 2.718281828459045;
    f = truncate_precision(f);
    printf("log(e) is about %f\n", log(f));
    return 0;
}
