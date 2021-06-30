#include <stdio.h>
#include <stdint.h>
#include <math.h>

typedef union MyFloat {
    float f;
    uint32_t i;
} MyFloat;

int main(void) {
    FILE *fp = NULL;
    MyFloat f;
    size_t i;

    fp = fopen("pow-lookup-table.bin", "wb");
    f.i = 0;

    for (i = 0; i < 32640; i++) {
        const float ans = log(f.f);
        f.i = i << 16;
        fwrite(&ans, sizeof(ans), 1, fp);
        //printf("%ld: log(%f) = %f\n", i, f.f, ans);
    }

    fclose(fp);
    return 0;
}
