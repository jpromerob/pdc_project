#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <omp.h>  // Include OpenMP header
#include <time.h>
#include <sys/time.h>  // For gettimeofday

#define WIDTH 640
#define HEIGHT 480
#define MILLIS 10000  // Number of milliseconds for the output array
#define EVENT_SIZE_BYTES 12  // Each event is 96 bits = 12 bytes

// #define DEBUGGER

#define MAX_FILENAME_LENGTH 256

// Function to extract base name without directory and extension
void get_base_name(const char *input_filename, char *base_name) {
    const char *last_slash = strrchr(input_filename, '/');
    const char *dot = strrchr(input_filename, '.');
    
    // If there is no slash, the base name starts from the beginning
    if (last_slash == NULL) {
        last_slash = input_filename - 1;
    }
    // If there is no dot, set the end of the base name to the end of the string
    if (dot == NULL || dot < last_slash) {
        dot = input_filename + strlen(input_filename);
    }
    
    // Copy base name excluding path and extension
    strncpy(base_name, last_slash + 1, dot - last_slash - 1);
    base_name[dot - last_slash - 1] = '\0';
}

// Function to print the current date and time with milliseconds
void print_current_datetime() {
    struct timeval tv;
    struct tm* local_time;
    
    // Get the current time with microsecond precision
    gettimeofday(&tv, NULL);
    
    // Convert the current time to local time
    local_time = localtime(&tv.tv_sec);

    // Print the date, time, and milliseconds
    printf(" *** *** *** %02d-%02d-%04d %02d:%02d:%02d.%03ld\n",
           local_time->tm_mday,
           local_time->tm_mon + 1,
           local_time->tm_year + 1900,
           local_time->tm_hour,
           local_time->tm_min,
           local_time->tm_sec,
           tv.tv_usec / 1000);  // Convert microseconds to milliseconds
}

int main(int argc, char *argv[]) {
    double all_start_time, all_end_time;
    all_start_time = omp_get_wtime();

    if (argc < 3) {
        printf("Usage: %s --file <bin_filename> [--threads <num_threads>]\n", argv[0]);
        return 1;
    }

    const char *bin_filename = NULL;
    int num_threads = 1;  // Default to 1 thread

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--file") == 0 && i + 1 < argc) {
            bin_filename = argv[i + 1];
        } else if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc) {
            num_threads = atoi(argv[i + 1]);  // Convert the number of threads from string to int
            if (num_threads <= 0) {
                printf("Error: Number of threads must be a positive integer.\n");
                return 1;
            }
        }
    }

    if (bin_filename == NULL) {
        printf("Error: BIN file not specified.\n");
        return 1;
    }

    // Extract base name from input file
    char base_name[MAX_FILENAME_LENGTH];
    get_base_name(bin_filename, base_name);

    // Construct the output file name
    char output_filename[MAX_FILENAME_LENGTH];
    snprintf(output_filename, sizeof(output_filename), "heatmaps/map_%s.bin", base_name);

    printf("Output File Name: %s\n", output_filename);

    // Open the BIN file
    FILE *file = fopen(bin_filename, "rb");
    if (!file) {
        printf("Error: Could not open input file %s\n", bin_filename);
        return 1;
    }

    // Read the total number of events (first 4 bytes)
    unsigned int total_events;
    fread(&total_events, sizeof(unsigned int), 1, file);
    printf("Total Events: %u\n", total_events);

    // Determine events per thread
    int events_per_thread = total_events / num_threads;
    if (total_events % num_threads != 0) {
        events_per_thread++;  // Handle any remaining events
    }

    printf("Events per Thread: %d\n", events_per_thread);

    // Create an array to store the start positions
    long *start_positions = malloc((num_threads + 1) * sizeof(long));
    if (!start_positions) {
        printf("Error: Memory allocation failed.\n");
        fclose(file);
        return 1;
    }

    // Calculate the starting byte position for each thread
    start_positions[0] = sizeof(unsigned int);  // First thread starts right after the total events count

    for (int i = 1; i <= num_threads; i++) {
        start_positions[i] = start_positions[i - 1] + events_per_thread * EVENT_SIZE_BYTES;
        if (start_positions[i] > sizeof(unsigned int) + total_events * EVENT_SIZE_BYTES) {
            start_positions[i] = sizeof(unsigned int) + total_events * EVENT_SIZE_BYTES;
        }
    }

    #ifdef DEBUGGER
        // Debug: Print start positions for each thread
        for (int i = 0; i < num_threads; i++) {
            printf("Start position for thread %d: %ld\n", i, start_positions[i]);
        }
        printf("End position: %ld\n", start_positions[num_threads]);
    #endif

    fclose(file);  // Close the shared file pointer since each thread will open its own

    // Set the number of OpenMP threads
    omp_set_num_threads(num_threads);

    // Allocate a contiguous block for the entire 3D matrix
    unsigned int ***heatmap_3d = (unsigned int ***)malloc(num_threads * sizeof(unsigned int **));
    unsigned int *data_block = (unsigned int *)malloc(num_threads * WIDTH * HEIGHT * sizeof(unsigned int));

    // Check if allocation was successful
    if (heatmap_3d == NULL || data_block == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Initialize the entire block to 0 using memset
    memset(data_block, 0, num_threads * WIDTH * HEIGHT * sizeof(unsigned int));

    // Set up the pointers for the 3D array
    for (int i = 0; i < num_threads; i++) {
        heatmap_3d[i] = (unsigned int **)malloc(WIDTH * sizeof(unsigned int *));
        for (int j = 0; j < WIDTH; j++) {
            // Each 2D slice points to the correct position in the contiguous block
            heatmap_3d[i][j] = data_block + (i * WIDTH * HEIGHT) + (j * HEIGHT);
        }
    }

    // START OF MAIN PROCESSING 
    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        long start_pos = start_positions[thread_id];
        long end_pos = start_positions[thread_id + 1];

        // Open the file separately in each thread
        FILE *local_file = fopen(bin_filename, "rb");
        if (!local_file) {
            printf("Error: Thread %d could not open file %s\n", thread_id, bin_filename);
        }

        // Move to the starting position of this thread's part
        fseek(local_file, start_pos, SEEK_SET);

        double start_time = omp_get_wtime();  // Measure time taken by each thread

        // Read the part assigned to this thread
        while (ftell(local_file) < end_pos) {
            uint64_t timestamp;
            unsigned short x, y;

            // Read each event: timestamp (64 bits) and x, y (16 bits each)
            fread(&timestamp, sizeof(uint64_t), 1, local_file);
            fread(&x, sizeof(unsigned short), 1, local_file);
            fread(&y, sizeof(unsigned short), 1, local_file);

            // Update heatmap_3d for the current thread
            heatmap_3d[thread_id][x][y]++;
        }

        double end_time = omp_get_wtime();  // End time measurement

        #ifdef DEBUGGER
            printf("Thread %d processed its part in %.6f seconds\n", thread_id, end_time - start_time);
        #endif

        // Close the file after processing is done
        fclose(local_file);
    }

    // Allocate a contiguous block for the 2D matrix
    unsigned int *heatmap_2d_block = (unsigned int *)malloc(WIDTH * HEIGHT * sizeof(unsigned int));
    unsigned int **heatmap_2d = (unsigned int **)malloc(WIDTH * sizeof(unsigned int *));

    // Initialize the entire block to 0 using memset
    memset(heatmap_2d_block, 0, WIDTH * HEIGHT * sizeof(unsigned int));

    // Set up the pointers for the 2D array
    for (int i = 0; i < WIDTH; i++) {
        heatmap_2d[i] = heatmap_2d_block + (i * HEIGHT);
    }

    // Sum heatmap_3d along the thread dimension
    for (int i = 0; i < num_threads; i++) {
        for (int x = 0; x < WIDTH; x++) {
            for (int y = 0; y < HEIGHT; y++) {
                heatmap_2d[x][y] += heatmap_3d[i][x][y];
            }
        }
    }

    // END OF MAIN PROCESSING 

    // Output the combined occurrences array
    FILE *output_file = fopen(output_filename, "wb");
    if (!output_file) {
        printf("Error: Could not create output file occurrences.bin\n");
        return 1;
    }

    fwrite(heatmap_2d_block, sizeof(unsigned int), WIDTH * HEIGHT, output_file);
    fclose(output_file);

    printf("Occurrences data saved to %s\n", output_filename);
    
    // Free allocated memory
    free(data_block);
    for (int i = 0; i < num_threads; i++) {
        free(heatmap_3d[i]);
    }
    free(heatmap_3d);

    free(heatmap_2d_block);
    free(heatmap_2d);
    free(start_positions);

    all_end_time = omp_get_wtime();
    printf("Elapsed time: %f seconds\n", all_end_time - all_start_time);

    return 0;
}
