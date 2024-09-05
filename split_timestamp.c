#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_LINE_LENGTH 256

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input_file> <duration>\n", argv[0]);
        return 1;
    }

    const char *input_filename = argv[1];
    int duration = atoi(argv[2]); // Convert argument to integer

    if (duration <= 0) {
        fprintf(stderr, "Error: Duration must be a positive integer.\n");
        return 1;
    }

    FILE *input_file = fopen(input_filename, "r");
    if (input_file == NULL) {
        perror("Error opening input file");
        return 1;
    }

    int file_count = 0;
    long long ev_count;
    FILE *output_file = NULL;

    long long first_timestamp = 0;
    long long timestamp;
    int x, y, polarity;

    bool check_first_t = true;


    //🌟
    char output_filename[64];
    snprintf(output_filename, sizeof(output_filename), "nothing.csv");

    while (fscanf(input_file, "%lld,%d,%d,%d\n", &timestamp, &x, &y, &polarity) == 4) {
        
        if (check_first_t) {
            first_timestamp = timestamp;
            check_first_t = false;
            ev_count=0;
        }

        // If this is the start of a new file or the first file
        if (timestamp > first_timestamp + duration*1000) {

            // Close the current output file if one is open
            if (output_file != NULL) {
                printf("Closing %s with  %lld events\n", output_filename, ev_count);
                fclose(output_file);
            }

            // Create a new output file
            snprintf(output_filename, sizeof(output_filename), "output_%d.csv", file_count++);
            output_file = fopen(output_filename, "w");
            if (output_file == NULL) {
                perror("Error creating output file");
                fclose(input_file);
                return 1;
            } else {
                printf("Opening %s\n", output_filename);
            }


            first_timestamp = timestamp;
            ev_count=0;

        } 


        // Modify the timestamp relative to the first timestamp
        // double modified_timestamp = timestamp - first_timestamp;

        // Write the modified line to the current output file
        
        if (output_file != NULL) {
            fprintf(output_file, "%lld,%d,%d,%d\n", (timestamp-first_timestamp), x, y, polarity);
            ev_count+=1;
        } 
    }

    // Close the last output file
    if (output_file != NULL) {
        printf("Closing %s with  %lld events (at last)\n", output_filename, ev_count);
        fclose(output_file);
    }

    fclose(input_file);
    printf("File split into %d files successfully.\n", file_count);
    return 0;
}
