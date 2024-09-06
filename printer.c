#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <N>\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);  // Convert the number of elements to print from string to int
    if (N <= 0) {
        printf("Error: N must be a positive integer.\n");
        return 1;
    }

    // Open the binary file for reading
    FILE *file = fopen("occurrences.bin", "rb");
    if (!file) {
        printf("Error: Could not open file occurrences.bin\n");
        return 1;
    }

    // Allocate memory to read N elements
    unsigned int *occurrences = malloc(N * sizeof(unsigned int));
    if (!occurrences) {
        printf("Error: Memory allocation failed.\n");
        fclose(file);
        return 1;
    }

    // Read the first N elements from the file
    size_t elements_read = fread(occurrences, sizeof(unsigned int), N, file);
    if (elements_read < N) {
        printf("Warning: Only %zu elements were read from the file.\n", elements_read);
    }

    // Close the file
    fclose(file);

    // Print the elements
    for (int i = 0; i < elements_read; i++) {
        printf("occurrences[%d] = %u\n", i, occurrences[i]);
    }

    // Free the allocated memory
    free(occurrences);
    return 0;
}
