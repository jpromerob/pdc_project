# FDD3260 Project

## Describing the pipeline
The aim of the project documented [here](https://github.com/jpromerob/pdc_project/blob/main/Report.pdf) was to generate, separately, histograms and heatmaps to analyze data from both temporal and spatial perspectives. A set of K×N files, each representing a 2-second recording of a specific scene, was processed. The approach to parallelization, illustrated in the Figure below, involved distributing the K×N files across N nodes, with each node handling K files sequentially. Each file was divided into M chunks of approximately equal size, allowing for parallel processing using OpenMP threads within each node. MPI was used to manage inter-process communication, while OpenMP facilitated parallel execution of tasks within each MPI process. Upon completion of all parallel tasks, the OpenMP parallel regions and the MPI environment were finalized.   

![alt_text](https://github.com/jpromerob/pdc_project/blob/main/images/Pipeline.PNG)

## Compiling the Code
[make](https://github.com/jpromerob/pdc_project/blob/main/Makefile)

## Running the tasks
[./run_mpi.sh](https://github.com/jpromerob/pdc_project/blob/main/run_mpi.sh)

## Checking Results

### Histograms Task
![alt text](https://github.com/jpromerob/pdc_project/blob/main/images/Results_Histograms.png)

### Heatmaps Task
![alt text](https://github.com/jpromerob/pdc_project/blob/main/images/Results_Heatmaps.png)
