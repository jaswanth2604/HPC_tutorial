#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void)
{
    const unsigned long long sizes[] = {10, 100, 10000, 1000000, 10000000,
                                        100000000, 1000000000};
    const int count = sizeof(sizes) / sizeof(sizes[0]);
    unsigned long long points = 0;
    unsigned long long inside = 0;

    // Seed the random number generator using the current time
    srand((unsigned int)time(NULL));

    printf("points\t\tinside\t\tpi estimate\ttime (seconds)\n");
    for (size_t i = 0; i < count; ++i) {
        struct timespec start, end;
        
        // Start the timer
        clock_gettime(CLOCK_MONOTONIC, &start);
        
        while (points < sizes[i]) {
            // rand() returns an integer between 0 and RAND_MAX.
            // Dividing by RAND_MAX gives a double between 0.0 and 1.0.
            // Multiplying by 2.0 and subtracting 1.0 maps it to the range [-1.0, 1.0].
            double x = 2.0 * ((double)rand() / RAND_MAX) - 1.0;
            double y = 2.0 * ((double)rand() / RAND_MAX) - 1.0;
            
            // Checking if the point falls inside the unit circle
            if (x * x + y * y <= 1.0)
                ++inside;
            ++points;
        }
        
        // Stop the timer
        clock_gettime(CLOCK_MONOTONIC, &end);
        
        // Calculate elapsed time
        double time_taken = (end.tv_sec - start.tv_sec) + 
                            (end.tv_nsec - start.tv_nsec) / 1e9;

        // Print the results including the time taken
        printf("%-12llu\t%-12llu\t%.10f\t%.6f\n", points, inside,
               4.0 * (double)inside / points, time_taken);
    }
    return 0;
}