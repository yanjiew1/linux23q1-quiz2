#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "cpucycles.h"

/* Prevent GCC to optimize out the body of count_utf8 and swar_count_utf8 */
static volatile size_t _count;

size_t count_utf8(const char *buf, size_t len)
{
    const int8_t *p = (const int8_t *) buf;
    size_t counter = 0;
    for (size_t i = 0; i < len; i++) {
        /* -65 is 0b10111111, anything larger in two-complement's should start
         * new code point.
         */
        if (p[i] > -65)
            counter++;
    }
    return counter;
}

size_t swar_count_utf8(const char *buf, size_t len)
{
    const uint64_t *qword = (const uint64_t *) buf;
    const uint64_t *end = qword + (len >> 3);

    size_t count = 0;
    for (; qword != end; qword++) {
        const uint64_t t0 = *qword;
        const uint64_t t1 = ~t0;
        const uint64_t t2 = t1 & 0x04040404040404040llu;
        const uint64_t t3 = t2 + t2;
        const uint64_t t4 = t0 & t3;
        count += __builtin_popcountll(t4);
    }

    count = (1 << 3) * (len / 8) - count;
    count += (len & 7) ? count_utf8((const char *) end, len & 7) : 0;

    return count;
}

#define TEST_SIZE 50000
#define NUM_TESTS 1000

uint64_t test_count_utf8(const char *buf, size_t len, size_t *count)
{
    uint64_t start = cpucycles();
    *count = count_utf8(buf, len);
    return cpucycles() - start;
}

uint64_t test_swar_count_utf8(const char *buf, size_t len, size_t *count)
{
    uint64_t start = cpucycles();
    *count = swar_count_utf8(buf, len);
    return cpucycles() - start;
}

int main(void)
{
    uint8_t buf[TEST_SIZE];
    for (size_t i = 0; i < TEST_SIZE; i++)
        buf[i] = rand();

    double t1 = 0, t2 = 0;
    size_t count1, count2;

    for (size_t i = 0; i < NUM_TESTS; i++) {
        t1 += test_count_utf8((const char *) buf, TEST_SIZE, &count1);
        t2 += test_swar_count_utf8((const char *) buf, TEST_SIZE, &count2);
    }

    t1 /= NUM_TESTS;
    t2 /= NUM_TESTS;

    /* Prevent GCC to optimize out the body of count_utf8 and swar_count_utf8 */
    _count = count1 + count2;

    printf("count_utf8:      %.2f\n", t1);
    printf("swar_count_utf8: %.2f\n", t2);

    return 0;
}
