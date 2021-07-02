#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

static float *tbl;
static const size_t TABLE_SIZE = 130560;
static int table_fd = 0;

float fastlogf(float x) {
    if (x < 0) {
        return NAN;
    } else if (x == 0) {
        return -INFINITY;
    } else {
        uint32_t index = 0;
        memcpy(&index, &x, sizeof(index));
        index = index >> 16;
        return tbl[index];
    }
}

float fastpowf(float x, float y) {
    return expf(y * fastlogf(x));
}

int fastmath_init() {
    table_fd = open("pow-lookup-table.bin", O_RDONLY);
    tbl = mmap(NULL, TABLE_SIZE, PROT_READ, MAP_SHARED, table_fd, 0);
    return tbl != MAP_FAILED;
}

void fastmath_close() {
    munmap(tbl, TABLE_SIZE);
    close(table_fd);
}

int main() {
    fastmath_init();
    printf("fastpowf(49, 0.5) = %f\n", fastpowf(49, 0.5));
    printf("fastpowf(49, -0.5) = %f\n", fastpowf(49, -0.5));
    printf("fastpowf(3243423.07, 0.025) = %f\n", fastpowf(3243423.07, 0.025));
    fastmath_close();
    return 0;
}
