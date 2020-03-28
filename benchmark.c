#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef SAMPLES
#define SAMPLES 1000000
#endif

void clflush(volatile void *Tx) { asm volatile("lfence;clflush (%0) \n" ::"c"(Tx)); }

static __inline__ uint64_t timer_start(void) {
    unsigned cycles_low, cycles_high;
    asm volatile(
            "CPUID\n\t"
            "RDTSC\n\t"
            "mov %%edx, %0\n\t"
            "mov %%eax, %1\n\t"
            : "=r"(cycles_high), "=r"(cycles_low)::"%rax", "%rbx", "%rcx", "%rdx");
    return ((uint64_t)cycles_high << 32) | cycles_low;
}

static __inline__ uint64_t timer_stop(void) {
    unsigned cycles_low, cycles_high;
    asm volatile(
            "RDTSCP\n\t"
            "mov %%edx, %0\n\t"
            "mov %%eax, %1\n\t"
            "CPUID\n\t"
            : "=r"(cycles_high), "=r"(cycles_low)::"%rax", "%rbx", "%rcx", "%rdx");
    return ((uint64_t)cycles_high << 32) | cycles_low;
}

static __inline__ void maccess(void *p) { asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax"); }

uint32_t reload(void *target) {
    volatile uint32_t time;
    uint64_t t1, t2;
    t1 = timer_start();
    maccess(target);
    t2 = timer_stop();
    return t2 - t1;
}

int main() {
    uint32_t data = 0xffffffff;
    uint32_t *unflush_times = malloc(sizeof(uint32_t) * SAMPLES);
    uint32_t *flushed_times = malloc(sizeof(uint32_t) * SAMPLES);
    for (int i = 0; i < SAMPLES; i++) {
        unflush_times[i] = reload(&data);
        clflush(&data);
        flushed_times[i] = reload(&data);
    }
    for (int i = 0; i < SAMPLES; i++) {
        printf("%d,Hit\n%d,Miss\n", unflush_times[i], flushed_times[i]);
    }
}
