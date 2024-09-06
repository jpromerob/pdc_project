#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    int num_threads = omp_get_max_threads();  // Default to the maximum number of available threads

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

    // Find the total number of lines in the file
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Read the very first timestamp
    unsigned long long first_timestamp = 0;
    char line[256];
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

    // Determine the size of each part for the threads
    long part_size = file_size / num_threads;

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
        long start_pos = thread_id * part_size;
        long end_pos = (thread_id == num_threads - 1) ? file_size : (thread_id + 1) * part_size;
        unsigned long long last_timestamp = 0;
        int is_first_row = 1;

        // Move to the starting position of this thread's part
        fseek(file, start_pos, SEEK_SET);

        // If not the first thread, adjust the start position to the beginning of the next line
        if (thread_id > 0) {
            char ch;
            while ((ch = fgetc(file)) != '\n' && ch != EOF);
        }

        double start_time = omp_get_wtime();  // Measure time taken by each thread

        // Read the part assigned to this thread
        while (ftell(file) < end_pos && fgets(line, sizeof(line), file)) {
            unsigned long long timestamp;
            int x, y;

            if (sscanf(line, "%llu,%d,%d,%*d", &timestamp, &x, &y) == 3) {
                if (is_first_row) {
                    is_first_row = 0;
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
    }

    fclose(file);

    // Combine the results from all threads
    for (int i = 0; i < num_threads; i++) {
        for (int j = 0; j < MILLIS; j++) {
            occurrences[j] += occurrences_private[i * MILLIS + j];
            if (occurrences_private[i * MILLIS + j]>0){
                printf("Thread %d: Element %d: %d\n", i, j, occurrences_private[i * MILLIS + j]);
            }
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
    return 0;
}
