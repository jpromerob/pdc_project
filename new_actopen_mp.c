#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <omp.h>  // Include OpenMP header

#define WIDTH 640
#define HEIGHT 480
#define MILLIS 1000  // Number of milliseconds for the output array

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s --file <csv_filename> [--threads <num_threads>]\n", argv[0]);
        return 1;
    }

    const char *csv_filename = NULL;
    int num_threads = 1;  // Default to 1 thread

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--file") == 0 && i + 1 < argc) {
            csv_filename = argv[i + 1];
        } else if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc) {
            num_threads = atoi(argv[i + 1]);  // Convert the number of threads from string to int
            if (num_threads <= 0) {
                printf("Error: Number of threads must be a positive integer.\n");
                return 1;
            }
        }
    }

    if (csv_filename == NULL) {
        printf("Error: CSV file not specified.\n");
        return 1;
    }

    // Open the CSV file
    FILE *file = fopen(csv_filename, "r");
    if (!file) {
        printf("Error: Could not open file %s\n", csv_filename);
        return 1;
    }

    // Step 1: Count total lines (M) in the CSV file
    int total_lines = 0;
    char line[1024]; // Buffer to store each line

    while (fgets(line, sizeof(line), file) != NULL) {
        total_lines++;
    }

    printf("Total Lines: %d\n", total_lines);

    // Step 2: Determine lines per thread (K)
    int lines_per_thread = total_lines / num_threads;
    if (total_lines % num_threads != 0) {
        lines_per_thread++;  // To handle any remaining lines
    }

    printf("Lines per Thread: %d\n", lines_per_thread);

    // Step 3: Create an array to store the start positions
    long *start_positions = malloc((num_threads + 1) * sizeof(long));

    if (!start_positions) {
        printf("Error: Memory allocation failed.\n");
        fclose(file);
        return 1;
    }

    // Step 4: Calculate the starting byte position for each thread
    rewind(file); // Reset the file pointer to the beginning of the file

    int current_line = 0;
    int current_thread = 0;
    start_positions[current_thread] = ftell(file); // Starting position of the first thread

    while (fgets(line, sizeof(line), file) != NULL) {
        if (current_line == (current_thread + 1) * lines_per_thread) {
            start_positions[++current_thread] = ftell(file);
        }
        current_line++;
    }

    rewind(file); // Reset the file pointer to the beginning of the file
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    start_positions[num_threads] = file_size;

    // Read the very first timestamp
    unsigned long long first_timestamp = 0;
    if (fgets(line, sizeof(line), file)) {
        unsigned long long timestamp;
        int x, y;
        if (sscanf(line, "%llu,%d,%d,%*d", &timestamp, &x, &y) == 3) {
            first_timestamp = timestamp;
        } else {
            printf("Error: Failed to read the first timestamp.\n");
            fclose(file);
            return 1;
        }
    }

    // Debug: Print start positions for each thread
    for (int i = 0; i < num_threads; i++) {
        printf("Start position for thread %d: %ld\n", i, start_positions[i]);
    }
    printf("End position: %ld\n", start_positions[num_threads]);

    fclose(file);  // Close the shared file pointer since each thread will open its own

    // Initialize arrays for counting occurrences
    unsigned int occurrences[MILLIS] = {0};  // Final array to store the results
    unsigned int *occurrences_private = malloc(num_threads * MILLIS * sizeof(unsigned int));  // Private arrays for each thread

    // Initialize private arrays to zero
    memset(occurrences_private, 0, num_threads * MILLIS * sizeof(unsigned int));

    // Set the number of OpenMP threads
    omp_set_num_threads(num_threads);

    // Start parallel processing
    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        long start_pos = start_positions[thread_id];
        long end_pos = start_positions[thread_id + 1];
        unsigned long long last_timestamp = 0;
        int is_first_row = 1;

        // Open the file separately in each thread
        FILE *local_file = fopen(csv_filename, "r");
        if (!local_file) {
            printf("Error: Thread %d could not open file %s\n", thread_id, csv_filename);
        }

        // Move to the starting position of this thread's part
        fseek(local_file, start_pos, SEEK_SET);

        double start_time = omp_get_wtime();  // Measure time taken by each thread

        // Read the part assigned to this thread
        while (ftell(local_file) < end_pos && fgets(line, sizeof(line), local_file)) {
            unsigned long long timestamp;
            int x, y;

            if (sscanf(line, "%llu,%d,%d,%*d", &timestamp, &x, &y) == 3) {
                if (is_first_row) {
                    is_first_row = 0;
                    printf("First row for Thread %i: %s\n", thread_id, line);
                }

                last_timestamp = timestamp;

                // Calculate the millisecond interval using the global first_timestamp
                unsigned int ms_interval = (timestamp - first_timestamp) / 1000;
                if (ms_interval < MILLIS) {
                    occurrences_private[thread_id * MILLIS + ms_interval]++;
                }
            }
        }

        double end_time = omp_get_wtime();  // End time measurement

        printf("Thread %d processed its part in %.6f seconds\n", thread_id, end_time - start_time);

        // Close the file after processing is done
        fclose(local_file);
    }

    // Combine the results from all threads
    for (int i = 0; i < num_threads; i++) {
        for (int j = 0; j < MILLIS; j++) {
            occurrences[j] += occurrences_private[i * MILLIS + j];
        }
    }

    // Output the combined occurrences array
    FILE *output_file = fopen("occurrences.bin", "wb");
    if (!output_file) {
        printf("Error: Could not create output file occurrences.bin\n");
        return 1;
    }

    fwrite(occurrences, sizeof(unsigned int), MILLIS, output_file);
    fclose(output_file);

    printf("Occurrences data saved to occurrences.bin\n");
    free(occurrences_private);
    free(start_positions);

    return 0;
}
