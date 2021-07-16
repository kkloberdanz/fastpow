/*
 * (C) 2021 Kyle Kloberdanz
 */

#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#define TIMEIT(stmt, time_buffer)                                             \
    do {                                                                      \
        struct timeval _ksm_start, _ksm_finish;                               \
        gettimeofday(&_ksm_start, NULL);                                      \
        {                                                                     \
            stmt;                                                             \
        }                                                                     \
        gettimeofday(&_ksm_finish, NULL);                                     \
        *time_buffer =                                                        \
            (1000000 * _ksm_finish.tv_sec + _ksm_finish.tv_usec) -            \
            (1000000 * _ksm_start.tv_sec + _ksm_start.tv_usec);               \
    } while (0)

static float *tbl;
static const size_t TABLE_SIZE = 130560;
static int table_fd = 0;

static void *check_malloc(size_t nbytes) {
    void *ptr = malloc(nbytes);
    if (!ptr) {
        perror("failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

float fastlogf(float x) {
    if (x < 0) {
        return NAN;
    } else if (x == 0) {
        return -INFINITY;
    } else {
        uint32_t index = 0;
        memcpy(&index, &x, sizeof(index));
        index = index >> 16;
        if (index > TABLE_SIZE) {
            return NAN;
        } else {
            return tbl[index];
        }
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

static double mse(float *actual, float *predicted, size_t nmemb) {
    double acc = 0.0;
    size_t i = 0;

    for (i = 0; i < nmemb; i++) {
        double residual = actual[i] - predicted[i];
        if (isnan(actual[i]) || isinf(actual[i])) {
            continue;
        }
        acc += residual * residual;
    }
    return acc / nmemb;
}

int main() {
#define N_FLOATS 100000000
    float *bases = NULL;
    float *powers = NULL;
    float *fastpow_results = NULL;
    float *libcpow_results = NULL;
    size_t i = 0;
    long runtime = 0;

    fastmath_init();
    srand(42);

    printf("fastpowf(49, 0.5) = %f\n", fastpowf(49, 0.5));
    printf("fastpowf(49, -0.5) = %f\n", fastpowf(49, -0.5));
    printf("fastpowf(3243423.07, 0.025) = %f\n", fastpowf(3243423.07, 0.025));

    bases = check_malloc(N_FLOATS * sizeof(float));
    powers = check_malloc(N_FLOATS * sizeof(float));
    fastpow_results = check_malloc(N_FLOATS * sizeof(float));
    libcpow_results = check_malloc(N_FLOATS * sizeof(float));

    for (i = 0; i < N_FLOATS; i++) {
        const float a = 1e10;
        float r1 = ((float)rand()/(float)(RAND_MAX)) * a;
        float r2 = ((float)rand()/(float)(RAND_MAX)) * a;

        r1 /= a;
        r2 /= a;

        /*printf("r1 = %f r2 = %f\n", r1, r2);*/
        bases[i] = r1;
        powers[i] = r2;
    }

    /* benchmark */
    TIMEIT(
        for (i = 0; i < N_FLOATS; i++) {
            libcpow_results[i] = powf(bases[i], powers[i]);
        },
        &runtime
    );
    printf("libc powf runtime: %ld us\n", runtime);

    TIMEIT(
        for (i = 0; i < N_FLOATS; i++) {
            fastpow_results[i] = fastpowf(bases[i], powers[i]);
        },
        &runtime
    );
    printf("fastpowf runtime:  %ld us\n", runtime);

    printf("MSE: %f\n", mse(libcpow_results, fastpow_results, N_FLOATS));

    /* cleanup */
    free(bases);
    free(fastpow_results);
    free(libcpow_results);

    fastmath_close();
    return 0;
#undef N_FLOATS
}
