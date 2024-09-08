# pdc_project

### Compiling
``` cc -DHEATMAPS -fopenmp -lmpi mpi_common_open_mp.c -o mpi_hm_open_mp.exe ```
``` cc -DHISTOGRAMS -fopenmp -lmpi mpi_common_open_mp.c -o mpi_hg_open_mp.exe ```

### Creating Histograms in C : Matrices in *.bin format 
``` srun -N 1 -A edu24.summer -t 00:10:00 -p main ./hg_open_mp.exe --file events/<event_file_name>.bin --threads <# threads> ```
``` srun -A edu24.summer -t 00:10:00 -p main --nodes=<MPI_processes> --ntasks-per-node=1 --cpus-per-task=<open_mp_threads> ./mpi_hg_open_mp.exe --folder <folder_name> ```

### Creating Heatmaps in C : Matrices in *.bin format 
``` srun -N 1 -A edu24.summer -t 00:10:00 -p main ./hm_open_mp.exe --file events/<event_file_name>.bin --threads <# threads> ```
``` srun -A edu24.summer -t 00:10:00 -p main --nodes=<MPI_processes> --ntasks-per-node=1 --cpus-per-task=<open_mp_threads> ./mpi_hm_open_mp.exe --folder <folder_name> ```

