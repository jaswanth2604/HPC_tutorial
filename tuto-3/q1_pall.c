#ifdef __linux__
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>

#ifdef __linux__
#include <sched.h>
#endif

// Set this to the number of PHYSICAL cores on your CPU for best performance.
// Hyperthreads/logical cores rarely help for a tight compute-bound loop like this.
#define NUM_THREADS 8

#define CACHE_LINE 64

/* ---------------------------------------------------------------------
 * Fast PRNG: xorshift128+ (the same generator used in old V8/JSC).
 * Two orders of magnitude cheaper per call than rand_r(), which goes
 * through libc, touches a lock-free but still non-trivial internal
 * state, and does an implicit modulo. xorshift128+ is ~6 integer ops,
 * fully inlinable, and trivially good enough for Monte Carlo sampling.
 * ------------------------------------------------------------------- */
typedef struct {
    uint64_t s0;
    uint64_t s1;
} rng_state_t;

static inline uint64_t xorshift128plus(rng_state_t *st) {
    uint64_t x = st->s0;
    const uint64_t y = st->s1;
    st->s0 = y;
    x ^= x << 23;
    x ^= x >> 17;
    x ^= y ^ (y >> 26);
    st->s1 = x;
    return x + y;
}

/* splitmix64 is used purely to turn an arbitrary 64-bit seed into a
 * well-mixed pair of non-zero state words for xorshift128+. */
static inline uint64_t splitmix64(uint64_t *seed) {
    uint64_t z = (*seed += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* ---------------------------------------------------------------------
 * Thread data, padded/aligned to a full cache line so that two threads
 * writing their own ThreadData never bounce the same cache line back
 * and forth between cores (false sharing).
 * ------------------------------------------------------------------- */
typedef struct {
    unsigned long long iterations;
    unsigned long long inside;
    uint64_t seed_s0;
    uint64_t seed_s1;
    int thread_index;
    char _pad[CACHE_LINE - (4 * sizeof(uint64_t)) - sizeof(int)]; // pad struct out to a full cache line
} __attribute__((aligned(CACHE_LINE))) ThreadData;

void* monte_carlo_worker(void* arg) {
    ThreadData* data = (ThreadData*)arg;

#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(data->thread_index % NUM_THREADS, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
#endif

    rng_state_t rng = { data->seed_s0, data->seed_s1 };
    // Discard the first couple of outputs; xorshift-family generators
    // mix a bit better once "warmed up" from a freshly seeded state.
    xorshift128plus(&rng);
    xorshift128plus(&rng);

    const unsigned long long iters = data->iterations;
    unsigned long long local_inside = 0;

    // 2^-32, used to map the top 32 bits of a 64-bit draw to [0, 1).
    const double norm = 1.0 / 4294967296.0;

    for (unsigned long long i = 0; i < iters; ++i) {
        // Use the TOP 32 bits of each draw: the low bits of xorshift-
        // style generators have shorter periods / weaker statistics,
        // the high bits are the good ones.
        uint32_t rx = (uint32_t)(xorshift128plus(&rng) >> 32);
        uint32_t ry = (uint32_t)(xorshift128plus(&rng) >> 32);

        double x = (double)rx * norm * 2.0 - 1.0; // -> [-1, 1)
        double y = (double)ry * norm * 2.0 - 1.0;

        // Branchless accumulate: lets -O3 vectorize/pipeline this loop
        // instead of paying a mispredict on a ~78%/22% biased branch.
        local_inside += (x * x + y * y <= 1.0);
    }

    data->inside = local_inside;
    return NULL;
}

int main(void) {
    const unsigned long long sizes[] = {10, 100, 10000, 1000000, 10000000,
                                         100000000, 1000000000};
    const int count = (int)(sizeof(sizes) / sizeof(sizes[0]));

    unsigned long long points = 0;
    unsigned long long inside = 0;

    pthread_t threads[NUM_THREADS];
    ThreadData t_data[NUM_THREADS];

    printf("point size\tpoints\t\tinside\t\tpi estimate\t\texecution time\n");

    for (int i = 0; i < count; ++i) {
        unsigned long long target = sizes[i];
        unsigned long long delta = target - points;

        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        unsigned long long points_per_thread = delta / NUM_THREADS;
        unsigned long long remainder = delta % NUM_THREADS;

        // High-resolution, per-iteration time source. (The original
        // time(NULL) has 1-second resolution, so on a fast machine
        // every outer loop iteration -- and hence every thread's seed
        // -- could collide and replay an identical random sequence.)
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        uint64_t time_seed = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;

        for (int t = 0; t < NUM_THREADS; ++t) {
            t_data[t].iterations = points_per_thread + ((unsigned long long)t < remainder ? 1 : 0);
            t_data[t].inside = 0;
            t_data[t].thread_index = t;

            // Unique per (outer iteration, thread) seed even if the
            // clock doesn't tick between calls.
            uint64_t seed = time_seed
                           ^ ((uint64_t)t * 0x9E3779B97F4A7C15ULL)
                           ^ ((uint64_t)i * 0xD1B54A32D192ED03ULL);

            t_data[t].seed_s0 = splitmix64(&seed);
            t_data[t].seed_s1 = splitmix64(&seed);
            if ((t_data[t].seed_s0 | t_data[t].seed_s1) == 0) {
                t_data[t].seed_s1 = 1; // xorshift128+ requires non-zero state
            }

            pthread_create(&threads[t], NULL, monte_carlo_worker, &t_data[t]);
        }

        unsigned long long delta_inside = 0;
        for (int t = 0; t < NUM_THREADS; ++t) {
            pthread_join(threads[t], NULL);
            delta_inside += t_data[t].inside;
        }

        inside += delta_inside;
        points += delta;
        clock_gettime(CLOCK_MONOTONIC, &end);

        double time_taken = (end.tv_sec - start.tv_sec) +
                    (end.tv_nsec - start.tv_nsec) / 1e9;
        printf("%llu\t\t%llu\t%llu\t%.10f\t%.6f seconds\n",
               target, points, inside,
               4.0 * (double)inside / (double)points, time_taken);
    }

    return 0;
}