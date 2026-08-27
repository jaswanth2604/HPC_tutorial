#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

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
    int n = (argc > 1) ? atoi(argv[1]) : 4096;
    int repeats = (argc > 2) ? atoi(argv[2]) : 3;
    size_t pixels;
    int *image;
    int *edges;
    int kernel[3][3] = {{-1, -1, -1}, {-1, 8, -1}, {-1, -1, -1}};
    double start, end;
    double seconds;

    if (n < 3 || repeats < 1) {
        fprintf(stderr, "Usage: %s [image_size>=3] [repeats>=1]\n", argv[0]);
        return EXIT_FAILURE;
    }

    pixels = (size_t)n * (size_t)n;
    image = (int *)malloc(pixels * sizeof(*image));
    edges = (int *)calloc(pixels, sizeof(*edges));
    if (image == NULL || edges == NULL) {
        fprintf(stderr, "Unable to allocate a %d x %d image.\n", n, n);
        free(image);
        free(edges);
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < pixels; ++i)
        image[i] = (int)(i % 256);

    start = now_seconds();
    for (int pass = 0; pass < repeats; ++pass) {
        for (int row = 1; row < n - 1; ++row) {
            for (int col = 1; col < n - 1; ++col) {
                int sum = 0;
                for (int kr = -1; kr <= 1; ++kr)
                    for (int kc = -1; kc <= 1; ++kc)
                        sum += image[(row + kr) * n + col + kc] * kernel[kr + 1][kc + 1];
                edges[row * n + col] = abs(sum);
            }
        }
    }
    end = now_seconds();
    seconds = end - start;

    printf("Question 2 - Serial edge detection\n");
    printf("Image: %d x %d, passes: %d\n", n, n, repeats);
    printf("Threads\tExecution time (seconds)\tChecksum\n");
    printf("1\t%.6f\t%lld\n", seconds,
           (long long)edges[(n / 2) * n + n / 2] * repeats);

    free(image);
    free(edges);
    return EXIT_SUCCESS;
}
