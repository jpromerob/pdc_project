#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include "mpi.h"

#define MAX_FILENAME_LENGTH 256
#define MAX_FILES 1000  // Adjust as needed

// Structure to store file names and count
typedef struct {
    char file_names[MAX_FILES][MAX_FILENAME_LENGTH];
    int nb_files;
} FileList;

int parse_arguments(int argc, char *argv[], const char **folder_path) {
    if (argc < 2) {
        printf("Usage: %s --folder <folder_path>\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--folder") == 0 && i + 1 < argc) {
            *folder_path = argv[i + 1];
            return 0;
        }
    }

    printf("Error: Folder path not specified.\n");
    return 1;
}

int scan_directory(const char *folder_path, FileList *file_list) {
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    dir = opendir(folder_path);
    if (dir == NULL) {
        perror("opendir");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {  // Regular file
            if (count < MAX_FILES) {
                strncpy(file_list->file_names[count], entry->d_name, MAX_FILENAME_LENGTH - 1);
                file_list->file_names[count][MAX_FILENAME_LENGTH - 1] = '\0';  // Ensure null-termination
                count++;
            } else {
                printf("Warning: Maximum number of files exceeded.\n");
                break;
            }
        }
    }

    closedir(dir);
    file_list->nb_files = count;
    return 0;
}

int main(int argc, char *argv[]) {
    const char *folder_path = NULL;
    FileList file_list = {0};  // Initialize with zero files
    int numtasks, rank, rc;

    // Initialize MPI
    rc = MPI_Init(&argc, &argv);
    if (rc != MPI_SUCCESS) {
        printf("Error starting MPI program. Terminating.\n");
        MPI_Abort(MPI_COMM_WORLD, rc);
    }

    // Get the number of MPI tasks and the rank of this process
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Process 0 parses arguments and scans the directory
    if (rank == 0) {
        if (parse_arguments(argc, argv, &folder_path) != 0) {
            MPI_Abort(MPI_COMM_WORLD, 1);  // Abort if error occurs
        }

        if (scan_directory(folder_path, &file_list) != 0) {
            MPI_Abort(MPI_COMM_WORLD, 1);  // Abort if error occurs
        }

        // Print the list of files found, only once by process 0
        printf("***************************\n");
        printf("Files found:\n");
        for (int i = 0; i < file_list.nb_files; i++) {
            printf("%s\n", file_list.file_names[i]);
        }
        printf("***************************\n");
    }

    // Broadcast the file list to all processes from process 0
    MPI_Bcast(&file_list, sizeof(FileList), MPI_BYTE, 0, MPI_COMM_WORLD);

    // Each MPI process prints its assigned file name
    if (rank < file_list.nb_files) {
        printf("Rank %d prints file: %s\n", rank, file_list.file_names[rank]);
    }

    // Synchronize and gather messages from all processes to process 0
    if (rank == 0) {
        // Collect messages from other processes
        printf("This is it my friend\n");
    }

    // Finalize MPI
    MPI_Finalize();
    return 0;
}
