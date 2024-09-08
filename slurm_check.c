#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("SLURM_JOB_NUM_NODES: %s\n", getenv("SLURM_JOB_NUM_NODES"));
    printf("SLURM_NTASKS: %s\n", getenv("SLURM_NTASKS"));
    printf("SLURM_CPUS_PER_TASK: %s\n", getenv("SLURM_CPUS_PER_TASK"));
    return 0;
}
