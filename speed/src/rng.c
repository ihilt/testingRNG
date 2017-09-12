#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include "aesctr.h"
#include "xorshift32.h"
#include "pcg32.h"
#include "xorshift128plus.h"
#include "xorshift1024star.h"
#include "xoroshiro128plus.h"
#include "splitmix64.h"
#include "pcg64.h"
#include "lehmer64.h"
#include "mersennetwister.h"
#include "mitchellmoore.h"
#include "xorshift-k4.h"
#include "xorshift-k5.h"
#include "widynski.h"

#ifndef __x86_64__
#warning "Expecting an x64 processor."
#endif

typedef uint32_t (*rand32fnc)(void);
typedef uint64_t (*rand64fnc)(void);
#define NUMBEROF32 8
rand32fnc our32[NUMBEROF32] = {xorshift_k4,   xorshift_k5, mersennetwister,
                               mitchellmoore, widynski, xorshift32,  pcg32,
                               rand};
const char *our32name[NUMBEROF32] = {
    "xorshift_k4",   "xorshift_k5", "mersennetwister",
    "mitchellmoore", "widynski", "xorshift32",  "pcg32",
    "rand"};

#define NUMBEROF64 7
rand64fnc our64[NUMBEROF64] = {aesctr,           lehmer64,   xorshift128plus,
                               xoroshiro128plus, splitmix64, pcg64, xorshift1024star};
const char *our64name[NUMBEROF64] = {"aesctr",          "lehmer64",
                                     "xorshift128plus", "xoroshiro128plus",
                                     "splitmix64",      "pcg64", "xorshift1024star"};

void populate32(rand32fnc rand, uint32_t *answer, size_t size) {
  for (size_t i = size; i != 0; i--) {
    answer[size - i] = rand();
  }
}

void populate64(rand64fnc rand, uint64_t *answer, size_t size) {
  for (size_t i = size; i != 0; i--) {
    answer[size - i] = rand();
  }
}

#define RDTSC_START(cycles)                                                    \
  do {                                                                         \
    register unsigned cyc_high, cyc_low;                                       \
    __asm volatile("cpuid\n\t"                                                 \
                   "rdtsc\n\t"                                                 \
                   "mov %%edx, %0\n\t"                                         \
                   "mov %%eax, %1\n\t"                                         \
                   : "=r"(cyc_high), "=r"(cyc_low)::"%rax", "%rbx", "%rcx",    \
                     "%rdx");                                                  \
    (cycles) = ((uint64_t)cyc_high << 32) | cyc_low;                           \
  } while (0)

#define RDTSC_FINAL(cycles)                                                    \
  do {                                                                         \
    register unsigned cyc_high, cyc_low;                                       \
    __asm volatile("rdtscp\n\t"                                                \
                   "mov %%edx, %0\n\t"                                         \
                   "mov %%eax, %1\n\t"                                         \
                   "cpuid\n\t"                                                 \
                   : "=r"(cyc_high), "=r"(cyc_low)::"%rax", "%rbx", "%rcx",    \
                     "%rdx");                                                  \
    (cycles) = ((uint64_t)cyc_high << 32) | cyc_low;                           \
  } while (0)

/*
 * Prints the best number of operations per cycle where
 * test is the function call, answer is the expected answer generated by
 * test, repeat is the number of times we should repeat and size is the
 * number of operations represented by test.
 */
#define BEST_TIME(test, testname, pre, repeat, size)                           \
  do {                                                                         \
    printf("%s: ", testname);                                                  \
    fflush(NULL);                                                              \
    uint64_t cycles_start, cycles_final, cycles_diff;                          \
    uint64_t min_diff = (uint64_t)-1;                                          \
    for (int i = 0; i < repeat; i++) {                                         \
      pre;                                                                     \
      __asm volatile("" ::: /* pretend to clobber */ "memory");                \
      RDTSC_START(cycles_start);                                               \
      test;                                                                    \
      RDTSC_FINAL(cycles_final);                                               \
      cycles_diff = (cycles_final - cycles_start);                             \
      if (cycles_diff < min_diff)                                              \
        min_diff = cycles_diff;                                                \
    }                                                                          \
    uint64_t S = size;                                                         \
    float cycle_per_op = (min_diff) / (double)S;                               \
    printf(" %.2f cycles per byte", cycle_per_op);                             \
    printf("\n");                                                              \
    fflush(NULL);                                                              \
  } while (0)

void demo(int size) {
  printf("Generating %d bytes of random numbers \n", size);
  printf("Time reported in number of cycles per byte.\n");
  printf("We store values to an array of size = %d kB.\n", size / (1024));
  int repeat = 500;
  void *prec = malloc(size);
  assert(size / 8 * 8 == size);
  printf("\nWe just generate the random numbers: \n");
  for (int k = 0; k < NUMBEROF32; k++)
    BEST_TIME(populate32(our32[k], prec, size / sizeof(uint32_t)), our32name[k],
              , repeat, size);
  for (int k = 0; k < NUMBEROF64; k++)
    BEST_TIME(populate64(our64[k], prec, size / sizeof(uint64_t)), our64name[k],
              , repeat, size);

  free(prec);
  printf("\n");
}

int main() {
  printf("\n");
  printf("We repeat the benchmark more than once. Make sure that you get "
         "comparable results.\n");
  demo(4096);

  demo(4096);
  return 0;
}
