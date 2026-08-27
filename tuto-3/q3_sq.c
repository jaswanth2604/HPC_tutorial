#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define PHILOSOPHERS 5

static double now_seconds(void)
{
#ifdef _WIN32
    static LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    if (frequency.QuadPart == 0) QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
#else
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)now.tv_sec + (double)now.tv_nsec / 1e9;
#endif
}

int main(int argc, char **argv)
{
    long meals = (argc > 1) ? atol(argv[1]) : 1000000L;
    volatile long eaten[PHILOSOPHERS] = {0};
    double start, end;

    if (meals < 1) {
        fprintf(stderr, "Usage: %s [meals_per_philosopher>=1]\n", argv[0]);
        return EXIT_FAILURE;
    }

    start = now_seconds();
    for (int philosopher = 0; philosopher < PHILOSOPHERS; ++philosopher)
        for (long meal = 0; meal < meals; ++meal)
            ++eaten[philosopher];
    end = now_seconds();

    printf("Question 3 - Sequential dining philosophers\n");
    printf("Philosophers: %d, meals each: %ld\n", PHILOSOPHERS, meals);
    printf("Execution time: %.6f seconds\n", end - start);
    printf("Total meals completed: %ld\n", eaten[0] + eaten[1] + eaten[2] + eaten[3] + eaten[4]);
    return EXIT_SUCCESS;
}
