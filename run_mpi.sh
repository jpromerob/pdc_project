#!/bin/bash

# Define different configurations
cpus_per_task_list=(2 4 8 16 32 48 64 96 128)
nodes_list=(1 2 3 4)

# Loop through configurations and run the MPI program
for nodes in "${nodes_list[@]}"; do
    for cpus_per_task in "${cpus_per_task_list[@]}"; do
        echo "Running with --nodes=$nodes and --cpus-per-task=$cpus_per_task"

        ############################################
        #                 Heatmaps                 #
        ############################################

        # Run the MPI program using GPU offload
        srun -A edu24.summer -t 00:10:00 -p gpu --nodes=$nodes --ntasks-per-node=1 --cpus-per-task=$cpus_per_task ./gpu_mpi_hm_open_mp.exe --folder events

        # Run the MPI program without GPU offload
        srun -A edu24.summer -t 00:10:00 -p gpu --nodes=$nodes --ntasks-per-node=1 --cpus-per-task=$cpus_per_task ./mpi_hm_open_mp.exe --folder events

        ############################################
        #                Histograms                #
        ############################################

        # Run the MPI program using GPU offload
        srun -A edu24.summer -t 00:10:00 -p gpu --nodes=$nodes --ntasks-per-node=1 --cpus-per-task=$cpus_per_task ./gpu_mpi_hg_open_mp.exe --folder events

        # Run the MPI program without GPU offload
        srun -A edu24.summer -t 00:10:00 -p gpu --nodes=$nodes --ntasks-per-node=1 --cpus-per-task=$cpus_per_task ./mpi_hg_open_mp.exe --folder events
    done
done
