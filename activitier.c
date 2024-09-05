#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 640
#define HEIGHT 480
#define MILLIS 1000  // Number of milliseconds for the output array

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s --file <csv_filename>\n", argv[0]);
        return 1;
    }

    const char *csv_filename = NULL;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--file") == 0 && i + 1 < argc) {
            csv_filename = argv[i + 1];
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

    // Initialize array to store occurrences per millisecond
    unsigned int occurrences[MILLIS] = {0};

    char line[256];
    unsigned long long first_timestamp = 0;
    unsigned long long last_timestamp = 0;
    int is_first_row = 1;

    // Read the CSV file line by line
    while (fgets(line, sizeof(line), file)) {
        unsigned long long timestamp;
        int x, y;

        if (sscanf(line, "%llu,%d,%d,%*d", &timestamp, &x, &y) == 3) {
            if (is_first_row) {
                first_timestamp = timestamp;
                is_first_row = 0;
            }

            last_timestamp = timestamp;

            // Calculate the millisecond interval
            unsigned int ms_interval = (timestamp - first_timestamp) / 1000;
            if (ms_interval < MILLIS) {
                occurrences[ms_interval]++;
            }
        }
    }
    fclose(file);

    // Output the occurrences array
    FILE *output_file = fopen("occurrences.bin", "wb");
    if (!output_file) {
        printf("Error: Could not create output file occurrences.bin\n");
        return 1;
    }

    fwrite(occurrences, sizeof(unsigned int), MILLIS, output_file);
    fclose(output_file);

    printf("Occurrences data saved to occurrences.bin\n");
    return 0;
}
