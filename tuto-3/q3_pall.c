#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif
#define PHILOSOPHERS 5

typedef struct { int id; long meals; long eaten; pthread_mutex_t *chopsticks; } PhilosopherArgs;

static double now_seconds(void)
{
#ifdef _WIN32
    static LARGE_INTEGER frequency; LARGE_INTEGER counter;
    if (frequency.QuadPart == 0) QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
#else
    struct timespec now; clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)now.tv_sec + (double)now.tv_nsec / 1e9;
#endif
}

static void *philosopher_worker(void *argument)
{
    PhilosopherArgs *p = argument;
    int left = p->id, right = (p->id + 1) % PHILOSOPHERS;
    int first = left < right ? left : right, second = left < right ? right : left;
    for (long meal = 0; meal < p->meals; ++meal) {
        pthread_mutex_lock(&p->chopsticks[first]);
        pthread_mutex_lock(&p->chopsticks[second]);
        ++p->eaten;
        pthread_mutex_unlock(&p->chopsticks[second]);
        pthread_mutex_unlock(&p->chopsticks[first]);
    }
    return NULL;
}

int main(int argc, char **argv)
{
    long meals = argc > 1 ? atol(argv[1]) : 1000000L, total = 0;
    pthread_t threads[PHILOSOPHERS]; PhilosopherArgs args[PHILOSOPHERS];
    pthread_mutex_t chopsticks[PHILOSOPHERS]; double start, end;
    if (meals < 1) return EXIT_FAILURE;
    for (int i = 0; i < PHILOSOPHERS; ++i) pthread_mutex_init(&chopsticks[i], NULL);
    start = now_seconds();
    for (int i = 0; i < PHILOSOPHERS; ++i) {
        args[i] = (PhilosopherArgs){i, meals, 0, chopsticks};
        pthread_create(&threads[i], NULL, philosopher_worker, &args[i]);
    }
    for (int i = 0; i < PHILOSOPHERS; ++i) { pthread_join(threads[i], NULL); total += args[i].eaten; }
    end = now_seconds();
    printf("Question 3 - Pthreads dining philosophers\nPhilosophers: %d, meals each: %ld\n", PHILOSOPHERS, meals);
    printf("Execution time: %.6f seconds\nTotal meals completed: %ld\n", end - start, total);
    for (int i = 0; i < PHILOSOPHERS; ++i) pthread_mutex_destroy(&chopsticks[i]);
    return EXIT_SUCCESS;
}
