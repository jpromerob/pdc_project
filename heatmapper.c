#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH 640
#define HEIGHT 480

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s --file <csv_filename>\n", argv[0]);
        return 1;
    }

    const char *csv_filename = NULL;
    const char *matrix_filename = "matrix.bin";  // Output binary file

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

    // Create and initialize the heatmap matrix
    unsigned int heatmap[WIDTH][HEIGHT] = {0};  // Stores counts for each pixel
    char line[256];

    // Read the CSV file line by line
    while (fgets(line, sizeof(line), file)) {
        int x, y;
        if (sscanf(line, "%*d,%d,%d,%*d", &x, &y) == 2) {
            if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                heatmap[x][y] += 1;
            }
        }
    }
    fclose(file);

    // Save the matrix to a binary file
    FILE *matrix_file = fopen(matrix_filename, "wb");
    if (!matrix_file) {
        printf("Error: Could not create output file %s\n", matrix_filename);
        return 1;
    }

    fwrite(heatmap, sizeof(unsigned int), WIDTH * HEIGHT, matrix_file);
    fclose(matrix_file);

    printf("Matrix data saved to %s\n", matrix_filename);
    return 0;
}
