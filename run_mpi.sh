#!/bin/bash
#SBATCH --job-name=mpi_openmp_job  # Job name
#SBATCH --account=edu24.summer     # Account name
#SBATCH --partition=main           # Partition name
#SBATCH --nodes=4                  # Number of nodes
#SBATCH --ntasks-per-node=1        # Number of tasks per node
#SBATCH --cpus-per-task=100        # Number of CPUs per task
#SBATCH --time=00:10:00            # Maximum run time (hh:mm:ss)
#SBATCH --output=output_mpi_openmp_%j.txt  # Output file name (%j will be replaced by job ID)


# Print the node list and job details
echo "Running on nodes:"
scontrol show hostname $SLURM_JOB_NODELIST
echo "Number of nodes: $SLURM_JOB_NUM_NODES"
echo "Tasks per node: $SLURM_NTASKS_PER_NODE"
echo "CPUs per task: $SLURM_CPUS_PER_TASK"

# Run the MPI program
srun ./mpi_hg_open_mp.exe --folder events
