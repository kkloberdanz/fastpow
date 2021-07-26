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
#include <emmintrin.h>
#include <immintrin.h>

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
static const float CLOSE_TO_ZERO = 1e-10;

static const float EXPF_DIVISORS[] __attribute__((aligned(32))) = {
    2.0f,
    6.0f,
    24.0f,
    120.0f,
    720.0f,
    5040.0f,
    40320.0f,
    362880.0f,
    3628800.0f,
    39916800.0f,
    479001600.0f,
    6227020800.0f,
    87178291200.0f,
    1307674368000.0f,
    20922789888000.0f,
    355687428096000.0f,
    6402373705728000.0f,
    121645100408832000.0f,
    2432902008176640000.0f,
    51090942171709440000.0f,
    1124000727777607680000.0f,
    25852016738884976640000.0f,
    620448401733239439360000.0f,
    15511210043330985984000000.0f,
    403291461126605635584000000.0f,
    10888869450418352160768000000.0f,
    304888344611713860501504000000.0f,
    8841761993739701954543616000000.0f,
    265252859812191058636308480000000.0f,
    8222838654177922817725562880000000.0f,
    263130836933693530167218012160000000.0f,
    8683317618811886495518194401280000000.0f
};

/*
 * Adapted from:
 * https://stackoverflow.com/a/49943540/5513419
 */
static float hsum_float_avx(__m256 v) {
    __m128 vlow;
    __m128 vhigh;
    __m128 high64;

    vlow = _mm256_castps256_ps128(v);
    vhigh = _mm256_extractf128_ps(v, 1); /* high 128 */
    vlow = _mm_add_ps(vlow, vhigh); /* reduce down to 128 */
    high64 = _mm_unpackhi_ps(vlow, vlow);
    return _mm_cvtss_f32(_mm_add_ss(vlow, high64)); /* reduce to scalar */
}

static float fastexpf(float x) {
    size_t i;
    float numerators[32] __attribute__((aligned(32)));
    float result[32] __attribute__((aligned(32)));

    double acc = 1.0 + x;

    /* order of operations with floats is important. It's best to
     * multiply terms of similar orders of magnitude to avoid loss in
     * precision. */
    float x2 = x * x;
    float x3 = x2 * x;
    float x4 = x2 * x2;
    float x5 = x2 * x3;
    float x6 = x3 * x3;
    float x7 = x4 * x3;
    float x8 = x4 * x4;
    float x9 = x5 * x4;
    float x10 = x5 * x5;
    float x11 = x6 * x5;
    float x12 = x6 * x6;
    float x13 = x7 * x6;
    float x14 = x7 * x7;
    float x15 = x8 * x7;
    float x16 = x8 * x8;
    float x17 = x9 * x8;
    float x18 = x9 * x9;
    float x19 = x10 * x9;
    float x20 = x10 * x10;
    float x21 = x11 * x10;
    float x22 = x11 * x11;
    float x23 = x12 * x11;
    float x24 = x12 * x12;
    float x25 = x13 * x12;
    float x26 = x13 * x13;
    float x27 = x14 * x13;
    float x28 = x14 * x14;
    float x29 = x15 * x14;
    float x30 = x15 * x15;
    float x31 = x16 * x15;
    float x32 = x16 * x16;
    float x33 = x17 * x16;

    numerators[0] = x2;
    numerators[1] = x3;
    numerators[2] = x4;
    numerators[3] = x5;
    numerators[4] = x6;
    numerators[5] = x7;
    numerators[6] = x8;
    numerators[7] = x9;
    numerators[8] = x10;
    numerators[9] = x11;
    numerators[10] = x12;
    numerators[11] = x13;
    numerators[12] = x14;
    numerators[13] = x15;
    numerators[14] = x16;
    numerators[15] = x17;
    numerators[16] = x18;
    numerators[17] = x19;
    numerators[18] = x20;
    numerators[19] = x21;
    numerators[20] = x22;
    numerators[21] = x23;
    numerators[22] = x24;
    numerators[23] = x25;
    numerators[24] = x26;
    numerators[25] = x27;
    numerators[26] = x28;
    numerators[27] = x29;
    numerators[28] = x30;
    numerators[29] = x31;
    numerators[30] = x32;
    numerators[31] = x33;

    for (i = 0; i < 32; i += 8) {
        const __m256 a = _mm256_load_ps(&numerators[i]);
        const __m256 b = _mm256_load_ps(&EXPF_DIVISORS[i]);
        const __m256 res = _mm256_div_ps(a, b);

        /* why are the results slightly wrong when using hsum_float_avx? */
        /*acc += hsum_float_avx(res);*/

        _mm256_stream_ps(&result[i], res);
    }

    for (i = 0; i < 32; i++) {
        acc += result[i];
    }
    return acc;
}

union FloatAsInt {
    uint32_t i;
    float f;
};

static float get_guess(float x) {
    const uint32_t exponent_mask = 0x3f800000;
    float guess;
    union FloatAsInt x_as_int;
    uint32_t exponent;

    x_as_int.f = x;
    exponent = (x_as_int.i & exponent_mask) >> 23;
    guess = exponent;
    return guess;
}

static float newtons_method_log(float x, float guess) {
    float y = guess;

    /* manual unrolling in this case was MUCH faster than a for loop */
    y = y - 1.0f + x / fastexpf(y);
    y = y - 1.0f + x / fastexpf(y);
    y = y - 1.0f + x / fastexpf(y);
    y = y - 1.0f + x / fastexpf(y);
    y = y - 1.0f + x / fastexpf(y);
    y = y - 1.0f + x / fastexpf(y);
    y = y - 1.0f + x / fastexpf(y);
    y = y - 1.0f + x / fastexpf(y);
    y = y - 1.0f + x / fastexpf(y);
    y = y - 1.0f + x / fastexpf(y);

    if (fabsf(y) < CLOSE_TO_ZERO) {
        return 0;
    } else {
        return y;
    }
}

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
    } else if (fabsf(x) < CLOSE_TO_ZERO) {
        return 1;
    } else {
        uint32_t index = 0;
        memcpy(&index, &x, sizeof(index));
        index = index >> 16;
        if (index > (TABLE_SIZE / sizeof(*tbl))) {
            return NAN;
        } else {
            /*const float guess = tbl[index];*/
            const float guess = get_guess(x);
            float y;
            /*printf("ln(%f): guess: %f actual: %f\n", x, guess, logf(x));*/
            y = newtons_method_log(x, guess);
            return y;
        }
    }
}

float fastpowf(float x, float y) {
    return fastexpf(y * fastlogf(x));
}

int fastmath_init() {
    if (!table_fd) {
        int success;
        table_fd = open("pow-lookup-table.bin", O_RDONLY);
        tbl = mmap(NULL, TABLE_SIZE, PROT_READ, MAP_SHARED, table_fd, 0);
        success = tbl != MAP_FAILED;
        if (success) {
            puts("mmap succeeded");
        } else {
            puts("mmap failed");
        }
        return success;
    } else {
        return -1;
    }
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
        acc += (residual * residual) / nmemb;
    }
    return acc;
}

int main() {
#define N_FLOATS 10000
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
        const float a = rand() % 100000;
        float r1 = ((float)rand()/(float)(RAND_MAX)) * a;
        float r2 = ((float)rand()/(float)(RAND_MAX)) * a;

        /*
        r1 /= a;
        r2 /= a;
        */

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

    TIMEIT(
        for (i = 0; i < N_FLOATS; i++) {
            fastpow_results[i] = fastlogf(bases[i]);
        },
        &runtime
    );
    printf("fastlogf runtime:  %ld us\n", runtime);

    TIMEIT(
        for (i = 0; i < N_FLOATS; i++) {
            fastpow_results[i] = logf(bases[i]);
        },
        &runtime
    );
    printf("logf runtime:  %ld us\n", runtime);

    /* cleanup */
    free(bases);
    free(powers);
    free(fastpow_results);
    free(libcpow_results);

    fastmath_close();
    return 0;
#undef N_FLOATS
}
