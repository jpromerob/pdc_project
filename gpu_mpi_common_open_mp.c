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
#define MILLIS 2000  // Number of milliseconds for the output array
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

// Function to find the file indices corresponding to a given rank
int* files_for_rank(int numfiles, int num_tasks, int rank, int* num_files_for_rank) {
    // Count the number of files assigned to this rank
    *num_files_for_rank = 0;
    for (int i = 0; i < numfiles; i++) {
        if (i % num_tasks == rank) {
            (*num_files_for_rank)++;
        }
    }

    // Allocate memory for the array to hold the file indices for this rank
    int* file_indices = (int*)malloc(*num_files_for_rank * sizeof(int));
    if (file_indices == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Fill the array with the file indices for this rank
    int idx = 0;
    for (int i = 0; i < numfiles; i++) {
        if (i % num_tasks == rank) {
            file_indices[idx++] = i;
        }
    }

    return file_indices;
}

int main(int argc, char *argv[]) {

    const char *folder_path = NULL;
    FileList file_list = {0};  // Initialize with zero files
    
    int num_tasks, rank, rc;
    double mpi_start_time, mpi_end_time, mpi_elapsed_time, mpi_max_elapsed_time;

    #ifdef HISTOGRAMS
        printf("This program creates Histograms\n");
    #endif

    #ifdef HEATMAPS
        printf("This program creates Heatmaps\n");
    #endif

    
    // Initialize MPI
    rc = MPI_Init(&argc, &argv);
    if (rc != MPI_SUCCESS) {
        printf("Error starting MPI program. Terminating.\n");
        MPI_Abort(MPI_COMM_WORLD, rc);
    }

    // Record the start time
    mpi_start_time = MPI_Wtime();

    // Get the number of MPI tasks and the rank of this process
    MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    printf("Available MPI tasks: %d\n", num_tasks);

    /********************************************************************************************/
    /*           PARSING ARGUMENTS, SETTING OUTPUT FILE NAMES, OPENING INPUT FILE, ETC          */
    /********************************************************************************************/


    int num_threads = 1;
    num_threads = omp_get_max_threads();  // Use maximum number of threads allowed by OpenMP runtime
    printf("Available OpenMP Threads: %d\n", num_threads);

    if (rank == 0) {
        if (parse_arguments(argc, argv, &folder_path) != 0) {
            return 1;
        }

        if (scan_directory(folder_path, &file_list) != 0) {
            return 1;
        }

        printf("Files found: %d\n", file_list.nb_files);
    }

    // Broadcast the file list to all processes from process 0
    MPI_Bcast(&file_list, sizeof(FileList), MPI_BYTE, 0, MPI_COMM_WORLD);


    
    int num_files_for_rank; // To hold the number of files for this rank
    int* file_idxs = files_for_rank(file_list.nb_files, num_tasks, rank, &num_files_for_rank);

    for (int i = 0; i < num_files_for_rank; i++) {

        
        printf("Rank %d : File %s\n", rank, file_list.file_names[file_idxs[i]]);
        
        char bin_filename[MAX_FILENAME_LENGTH] = {0};

        strncpy(bin_filename, file_list.file_names[file_idxs[i]], MAX_FILENAME_LENGTH - 1);

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

        
        uint64_t first_timestamp = 0;
        fread(&first_timestamp, sizeof(uint64_t), 1, file);
        #ifdef DEBUGGER
            printf("*** Very First Timestamp: %lu\n", first_timestamp);
        #endif

                
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
        
        // Set the number of OpenMP threads
        omp_set_num_threads(num_threads);

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

                int is_first_row = 1;

                fseek(local_file, start_pos, SEEK_SET);

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

                    uint64_t ms_interval = (timestamp - first_timestamp) / 1000;
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


            /****************************************************************************************/
            /*                            OFFLOADING ARRAY OPERATION TO GPU                         */
            /****************************************************************************************/
            #ifdef OFFLOADGPU
            #pragma omp target data map(to: occurrences_private[0:num_threads * MILLIS]) \
                                map(from: occurrences[0:MILLIS])
            {
                
                #pragma omp target teams distribute parallel for reduction(+:occurrences[:MILLIS])
            #endif // OFFLOADGPU
                for (int j = 0; j < MILLIS; j++) {
                    unsigned int sum = 0;
                    for (int i = 0; i < num_threads; i++) {
                        sum += occurrences_private[i * MILLIS + j];
                    }
                    occurrences[j] = sum;
                    #ifdef DEBUGGER
                        if(j > MILLIS/num_threads*99/100 && j < MILLIS/num_threads*101/100){
                            printf("Total occurrences at [ms] %d: %d\n", j, sum);
                        }
                    #endif
                }
            #ifdef OFFLOADGPU
            }
            #endif // OFFLOADGPU
        #endif

        #ifdef HEATMAPS
            // Allocate a contiguous block for the 2D matrix
            unsigned int *data_block_2d = (unsigned int *)malloc(WIDTH * HEIGHT * sizeof(unsigned int));
            unsigned int **heatmap_2d = (unsigned int **)malloc(WIDTH * sizeof(unsigned int *));
            
            memset(data_block_2d, 0, WIDTH * HEIGHT * sizeof(unsigned int));

            // Set up the pointers for the 2D array
            for (int i = 0; i < WIDTH; i++) {
                heatmap_2d[i] = data_block_2d + (i * HEIGHT);
            }

            /****************************************************************************************/
            /*                            OFFLOADING MATRIX OPERATION TO GPU                        */
            /****************************************************************************************/
            
            #ifdef OFFLOADGPU
            // Map the base pointers and the data blocks to the GPU
            #pragma omp target data map(to: heatmap_3d[0:num_threads]) \
                                map(to: data_block_3d[0:num_threads * WIDTH * HEIGHT]) \
                                map(from: data_block_2d[0:WIDTH * HEIGHT])
            {
                // Offload computation to GPU
                #pragma omp target teams distribute parallel for collapse(2)
            #endif // OFFLOADGPU
                for (int x = 0; x < WIDTH; x++) {
                    for (int y = 0; y < HEIGHT; y++) {
                        unsigned int sum = 0;
                        for (int i = 0; i < num_threads; i++) {
                            // Use directly the data block to avoid potential issues with pointer dereferencing
                            sum += data_block_3d[(i * WIDTH * HEIGHT) + (x * HEIGHT) + y];
                        }
                        data_block_2d[x * HEIGHT + y] = sum;
                    }
                }
            #ifdef OFFLOADGPU
            }
            #endif // OFFLOADGPU

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

    free(file_idxs); // Free the allocated memory

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

    // Synchronize and gather messages from all processes to process 0
    if (rank == 0) {
        // Collect messages from other processes
        printf("This is it my friend\n");
        
        #ifdef HISTOGRAMS
        const char *file_name = "summary_histograms.csv";
        #endif
        
        #ifdef HEATMAPS
        const char *file_name = "summary_heatmaps.csv";
        #endif

        // Open the CSV file in append mode
        FILE *summary_file = fopen(file_name, "a");
        if (summary_file == NULL) {
            perror("Error opening file");
            return EXIT_FAILURE;
        }

        int gpu_flag = 0;    
        #ifdef OFFLOADGPU
        gpu_flag = 1;
        #endif // OFFLOADGPU

        // Append the random data to the CSV file
        fprintf(summary_file, "%d,%d,%d,%.6f\n", num_tasks, num_threads, gpu_flag, mpi_max_elapsed_time);

        // Close the file
        fclose(summary_file);
    }

    // Finalize MPI
    MPI_Finalize();



    return 0;
}
