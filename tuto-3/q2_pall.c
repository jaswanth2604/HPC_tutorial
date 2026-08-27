#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

typedef struct { const int *image; int *edges; int n; int repeats; int first_row; int last_row; } WorkerArgs;

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

static void *edge_worker(void *argument)
{
    WorkerArgs *args = argument;
    static const int kernel[3][3] = {{-1,-1,-1},{-1,8,-1},{-1,-1,-1}};
    for (int pass = 0; pass < args->repeats; ++pass)
        for (int row = args->first_row; row < args->last_row; ++row)
            for (int col = 1; col < args->n - 1; ++col) {
                int sum = 0;
                for (int kr = -1; kr <= 1; ++kr)
                    for (int kc = -1; kc <= 1; ++kc)
                        sum += args->image[(row + kr) * args->n + col + kc] * kernel[kr + 1][kc + 1];
                args->edges[row * args->n + col] = abs(sum);
            }
    return NULL;
}

static double run_configuration(const int *image, int *edges, int n, int repeats, int requested)
{
    pthread_t *threads = malloc((size_t)requested * sizeof(*threads));
    WorkerArgs *args = malloc((size_t)requested * sizeof(*args));
    int actual = requested < n - 2 ? requested : n - 2;
    int base = (n - 2) / actual, extra = (n - 2) % actual, next = 1;
    double start = now_seconds();
    if (threads == NULL || args == NULL) { free(threads); free(args); return -1.0; }
    for (int t = 0; t < actual; ++t) {
        int rows = base + (t < extra ? 1 : 0);
        args[t] = (WorkerArgs){image, edges, n, repeats, next, next + rows};
        if (pthread_create(&threads[t], NULL, edge_worker, &args[t]) != 0) return -1.0;
        next += rows;
    }
    for (int t = 0; t < actual; ++t) pthread_join(threads[t], NULL);
    free(threads); free(args);
    return now_seconds() - start;
}

int main(int argc, char **argv)
{
    int n = argc > 1 ? atoi(argv[1]) : 4096, repeats = argc > 2 ? atoi(argv[2]) : 3;
    const int counts[] = {1,2,4,8,16}; size_t pixels = (size_t)n * (size_t)n;
    int *image, *edges; double serial_time;
    if (n < 3 || repeats < 1) return EXIT_FAILURE;
    image = malloc(pixels * sizeof(*image)); edges = calloc(pixels, sizeof(*edges));
    if (image == NULL || edges == NULL) { free(image); free(edges); return EXIT_FAILURE; }
    for (size_t i = 0; i < pixels; ++i) image[i] = (int)(i % 256);
    serial_time = run_configuration(image, edges, n, repeats, 1);
    printf("Question 2 - Pthreads edge detection\nImage: %d x %d, passes: %d\n", n, n, repeats);
    printf("Threads\tExecution time (seconds)\tSpeedup\tEfficiency\tChecksum\n");
    for (size_t i = 0; i < sizeof(counts)/sizeof(counts[0]); ++i) {
        double seconds = run_configuration(image, edges, n, repeats, counts[i]);
        printf("%d\t%.6f\t%.3f\t%.3f\t%lld\n", counts[i], seconds, serial_time / seconds,
               serial_time / (seconds * counts[i]), (long long)edges[(n/2)*n+n/2] * repeats);
    }
    printf("Serial baseline: %.6f seconds\n", serial_time);
    free(image); free(edges); return EXIT_SUCCESS;
}
