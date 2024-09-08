#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <omp.h>  // Include OpenMP header
#include <time.h>
#include <sys/time.h>  // For gettimeofday
#include <dirent.h>
#include <sys/types.h>
#include "mpi.h"

#define WIDTH 640
#define HEIGHT 480
#define MILLIS 10000  // Number of milliseconds for the output array
#define EVENT_SIZE_BYTES 12  // Each event is 96 bits = 12 bytes
#define MAX_FILENAME_LENGTH 256
#define MAX_FOLDER_LENGTH 32
#define MAX_FILES 1000  // Max Expected Nbr of Files

// Structure to store file names and count
typedef struct {
    char file_names[MAX_FILES][MAX_FILENAME_LENGTH];
    char folder_name[MAX_FOLDER_LENGTH];
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

    strncpy(file_list->folder_name, folder_path, MAX_FOLDER_LENGTH - 1);

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

// Function implementations
void get_base_name(const char *input_filename, char *base_name) {
    const char *last_slash = strrchr(input_filename, '/');
    const char *dot = strrchr(input_filename, '.');
    
    if (last_slash == NULL) {
        last_slash = input_filename - 1;
    }
    if (dot == NULL || dot < last_slash) {
        dot = input_filename + strlen(input_filename);
    }
    
    strncpy(base_name, last_slash + 1, dot - last_slash - 1);
    base_name[dot - last_slash - 1] = '\0';
}


void calculate_start_positions(int num_threads, unsigned int total_events, long **start_positions) {
    *start_positions = malloc((num_threads + 1) * sizeof(long));
    if (!*start_positions) {
        printf("Error: Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

    int events_per_thread = total_events / num_threads;
    if (total_events % num_threads != 0) {
        events_per_thread++;
    }

    (*start_positions)[0] = sizeof(unsigned int);
    for (int i = 1; i <= num_threads; i++) {
        (*start_positions)[i] = (*start_positions)[i - 1] + events_per_thread * EVENT_SIZE_BYTES;
        if ((*start_positions)[i] > sizeof(unsigned int) + total_events * EVENT_SIZE_BYTES) {
            (*start_positions)[i] = sizeof(unsigned int) + total_events * EVENT_SIZE_BYTES;
        }
    }
}


int main(int argc, char *argv[]) {

    const char *folder_path = NULL;
    FileList file_list = {0};  // Initialize with zero files
    
    int numtasks, rank, rc;
    double mpi_start_time, mpi_end_time, mpi_elapsed_time, mpi_max_elapsed_time;

    

    
    // Initialize MPI
    rc = MPI_Init(&argc, &argv);
    if (rc != MPI_SUCCESS) {
        printf("Error starting MPI program. Terminating.\n");
        MPI_Abort(MPI_COMM_WORLD, rc);
    }

    // Record the start time
    mpi_start_time = MPI_Wtime();

    // Get the number of MPI tasks and the rank of this process
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /********************************************************************************************/
    /*           PARSING ARGUMENTS, SETTING OUTPUT FILE NAMES, OPENING INPUT FILE, ETC          */
    /********************************************************************************************/



    if (rank == 0) {
        if (parse_arguments(argc, argv, &folder_path) != 0) {
            return 1;
        }

        if (scan_directory(folder_path, &file_list) != 0) {
            return 1;
        }

        printf("Files found:\n");
        for (int i = 0; i < file_list.nb_files; i++) {
            printf("%s\n", file_list.file_names[i]);
        }
    }

    // Broadcast the file list to all processes from process 0
    MPI_Bcast(&file_list, sizeof(FileList), MPI_BYTE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&file_list, sizeof(FileList), MPI_BYTE, 0, MPI_COMM_WORLD);


    // Each MPI process prints its assigned file name
    if (rank < file_list.nb_files) {
        
        char bin_filename[MAX_FILENAME_LENGTH] = {0};

        strncpy(bin_filename, file_list.file_names[rank], MAX_FILENAME_LENGTH - 1);

        // Extract base name from input file
        
        char base_name[MAX_FILENAME_LENGTH];
        get_base_name(bin_filename, base_name);

        char input_filename[MAX_FILENAME_LENGTH];
        snprintf(input_filename, sizeof(input_filename), "%s/%s", file_list.folder_name, bin_filename);
        

        double all_start_time;
        all_start_time = omp_get_wtime();


        // Construct the output file name
        
        char output_filename[MAX_FILENAME_LENGTH];
        #ifdef HISTOGRAMS
            snprintf(output_filename, sizeof(output_filename), "histograms/occ_%s.bin", base_name);
        #endif
        
        #ifdef HEATMAPS
            snprintf(output_filename, sizeof(output_filename), "heatmaps/map_%s.bin", base_name);
        #endif

        printf("Output File Name: %s\n", output_filename);

        // Open the BIN file
        FILE *file = fopen(input_filename, "rb");
        if (!file) {
            printf("Error: Could not open input file %s\n", input_filename);
            return 1;
        }

        unsigned int total_events;
        fread(&total_events, sizeof(unsigned int), 1, file);
        printf("Total Events: %u\n", total_events);

        
        int num_threads = 1;
        num_threads = omp_get_max_threads();  // Use maximum number of threads allowed by OpenMP runtime
        printf("Available OpenMP Threads: %d\n", num_threads);

        
        long *start_positions;
        calculate_start_positions(num_threads, total_events, &start_positions);

        fclose(file);  // Close the shared file pointer since each thread will open its own


        
        /********************************************************************************************/
        /*              PREPARING SHARED MEMORIES FOR PARALLEL-PROCESSING WITH OPEN_MP              */
        /********************************************************************************************/

        #ifdef HISTOGRAMS
            // Initialize arrays for counting occurrences
            unsigned int *occurrences_private = malloc(num_threads * MILLIS * sizeof(unsigned int));  // Private arrays for each thread
            memset(occurrences_private, 0, num_threads * MILLIS * sizeof(unsigned int));
        #endif


        #ifdef HEATMAPS

            // Allocate a contiguous block for the entire 3D matrix
            unsigned int ***heatmap_3d = (unsigned int ***)malloc(num_threads * sizeof(unsigned int **));
            unsigned int *data_block_3d = (unsigned int *)malloc(num_threads * WIDTH * HEIGHT * sizeof(unsigned int));

            // Check if allocation was successful
            if (heatmap_3d == NULL || data_block_3d == NULL) {
                fprintf(stderr, "Memory allocation failed\n");
                exit(EXIT_FAILURE);
            }

            // Initialize the entire block to 0 using memset
            memset(data_block_3d, 0, num_threads * WIDTH * HEIGHT * sizeof(unsigned int));

            // Set up the pointers for the 3D array
            for (int i = 0; i < num_threads; i++) {
                heatmap_3d[i] = (unsigned int **)malloc(WIDTH * sizeof(unsigned int *));
                for (int j = 0; j < WIDTH; j++) {
                    // Each 2D slice points to the correct position in the contiguous block
                    heatmap_3d[i][j] = data_block_3d + (i * WIDTH * HEIGHT) + (j * HEIGHT);
                }
            }

        #endif




        
        /********************************************************************************************/
        /*                            PARALLEL-PROCESSING WITH OPEN_MP                              */
        /********************************************************************************************/
        
        #pragma omp parallel // START OF MAIN PROCESSING
        {
            double start_time = omp_get_wtime();  // Measure time taken by each thread

            int thread_id = omp_get_thread_num();
            long start_pos = start_positions[thread_id];
            long end_pos = start_positions[thread_id + 1];

            // Open the file separately in each thread
            FILE *local_file = fopen(input_filename, "rb");
            if (!local_file) {
                printf("Error: Thread %d could not open file %s\n", thread_id, input_filename);
            }

            // Move to the starting position of this thread's part
            fseek(local_file, start_pos, SEEK_SET);
            
            /* Getting First Timestamp (to create [ms] bins) */ 
            #ifdef HISTOGRAMS

                uint64_t first_timestamp = 0;
                int is_first_row = 1;

                if (thread_id == 0) {
                    fread(&first_timestamp, sizeof(uint64_t), 1, local_file);
                    #ifdef DEBUGGER
                        printf("*** Very First Timestamp: %lu\n", first_timestamp);
                    #endif
                } else {
                    fseek(local_file, start_pos, SEEK_SET);
                }

                rewind(local_file);
                fseek(local_file, start_pos, SEEK_SET);

            #endif


            /* Read the part assigned to this thread */
            while (ftell(local_file) < end_pos) {
                uint64_t timestamp;
                unsigned short x, y;

                // Read each event: timestamp (64 bits) and x, y (16 bits each)
                fread(&timestamp, sizeof(uint64_t), 1, local_file);
                fread(&x, sizeof(unsigned short), 1, local_file);
                fread(&y, sizeof(unsigned short), 1, local_file);


                #ifdef HISTOGRAMS            
                    if (is_first_row) {
                        is_first_row = 0;
                    }

                    unsigned int ms_interval = (timestamp - first_timestamp) / 1000;
                    if (ms_interval < MILLIS) {
                        occurrences_private[thread_id * MILLIS + ms_interval]++;
                    }
                #endif

                #ifdef HEATMAPS
                    // Update heatmap_3d for the current thread
                    heatmap_3d[thread_id][x][y]++;
                #endif
            }

            double end_time = omp_get_wtime();  // End time measurement

            #ifdef DEBUGGER
                printf("Thread %d processed its part in %.6f seconds\n", thread_id, end_time - start_time);
            #endif

            // Close the file after processing is done
            fclose(local_file);

        } // End of OpenMP Parallel Processing


        
        /********************************************************************************************/
        /*                       CONSOLIDATING DATA FROM OPEN_MP PROCESSES                          */
        /********************************************************************************************/

        #ifdef HISTOGRAMS
        unsigned int occurrences[MILLIS] = {0};  // Final array to store the results
        // Combine the results from all threads
        for (int i = 0; i < num_threads; i++) {
            for (int j = 0; j < MILLIS; j++) {
                occurrences[j] += occurrences_private[i * MILLIS + j];
            }
        }
        #endif

        #ifdef HEATMAPS
            // Allocate a contiguous block for the 2D matrix
            unsigned int *data_block_2d = (unsigned int *)malloc(WIDTH * HEIGHT * sizeof(unsigned int));
            unsigned int **heatmap_2d = (unsigned int **)malloc(WIDTH * sizeof(unsigned int *));

            // Initialize the entire block to 0 using memset
            memset(data_block_2d, 0, WIDTH * HEIGHT * sizeof(unsigned int));

            // Set up the pointers for the 2D array
            for (int i = 0; i < WIDTH; i++) {
                heatmap_2d[i] = data_block_2d + (i * HEIGHT);
            }

            // Sum heatmap_3d along the thread dimension
            for (int i = 0; i < num_threads; i++) {
                for (int x = 0; x < WIDTH; x++) {
                    for (int y = 0; y < HEIGHT; y++) {
                        heatmap_2d[x][y] += heatmap_3d[i][x][y];
                    }
                }
            }
        #endif

        
        /********************************************************************************************/
        /*                               SAVING DATA INTO BINARY FILE                               */
        /********************************************************************************************/
        FILE *output_file = fopen(output_filename, "wb");
        if (!output_file) {
            printf("Error: Could not create output file.bin\n");
            return 1;
        }

        #ifdef HISTOGRAMS
            fwrite(occurrences, sizeof(unsigned int), MILLIS, output_file);
        #endif

        #ifdef HEATMAPS
            fwrite(data_block_2d, sizeof(unsigned int), WIDTH * HEIGHT, output_file);
        #endif
        fclose(output_file);
        printf("Data saved to %s\n", output_filename);
        

        /********************************************************************************************/
        /*                                 FREEING ALLOCATED MEMORY                                 */
        /********************************************************************************************/
        #ifdef HISTOGRAMS
            free(occurrences_private);
        #endif

        #ifdef HEATMAPS
            free(data_block_3d);
            for (int i = 0; i < num_threads; i++) {
                free(heatmap_3d[i]);
            }
            free(heatmap_3d);
            free(data_block_2d);
            free(heatmap_2d);
        #endif
        free(start_positions);

        /********************************************************************************************/
        /*                                   PRINTING ELAPSED TIME                                  */
        /********************************************************************************************/
        double all_end_time = omp_get_wtime();
        printf("\n *** Elapsed time for rank %i: %f seconds\n\n", rank, all_end_time - all_start_time);
    }

    // Synchronize and gather messages from all processes to process 0
    if (rank == 0) {
        // Collect messages from other processes
        printf("This is it my friend\n");
    }

    // Record the end time
    mpi_end_time = MPI_Wtime();

    // Calculate elapsed time for each process
    mpi_elapsed_time = mpi_end_time - mpi_start_time;

    // Use MPI_Reduce to find the maximum elapsed time among all processes
    MPI_Reduce(&mpi_elapsed_time, &mpi_max_elapsed_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    
    // Print the maximum elapsed time in the sequential part (only rank 0 prints)
    if (rank == 0) {
        printf("***************************************************\n");
        printf("      Maximum elapsed time: %f seconds\n", mpi_max_elapsed_time);
        printf("***************************************************\n");
    }

    // Finalize MPI
    MPI_Finalize();

    return 0;
}
