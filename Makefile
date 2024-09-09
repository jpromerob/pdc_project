# Makefile for compiling GPU and MPI programs

CC = cc
CFLAGS = -fopenmp -lmpi
SRC = gpu_mpi_common_open_mp.c

# Define executable names
EXE_GPU_MPI_HG_OPEN_MP = gpu_mpi_hg_open_mp.exe
EXE_GPU_MPI_HM_OPEN_MP = gpu_mpi_hm_open_mp.exe
EXE_MPI_HG_OPEN_MP = mpi_hg_open_mp.exe
EXE_MPI_HM_OPEN_MP = mpi_hm_open_mp.exe

# Targets
all: $(EXE_GPU_MPI_HG_OPEN_MP) $(EXE_GPU_MPI_HM_OPEN_MP) $(EXE_MPI_HG_OPEN_MP) $(EXE_MPI_HM_OPEN_MP)

$(EXE_GPU_MPI_HG_OPEN_MP): $(SRC)
	$(CC) -DOFFLOADGPU -DHISTOGRAMS $(CFLAGS) $< -o $@

$(EXE_GPU_MPI_HM_OPEN_MP): $(SRC)
	$(CC) -DOFFLOADGPU -DHEATMAPS $(CFLAGS) $< -o $@

$(EXE_MPI_HG_OPEN_MP): $(SRC)
	$(CC) -DHISTOGRAMS $(CFLAGS) $< -o $@

$(EXE_MPI_HM_OPEN_MP): $(SRC)
	$(CC) -DHEATMAPS $(CFLAGS) $< -o $@

clean:
	rm -f $(EXE_GPU_MPI_HG_OPEN_MP) $(EXE_GPU_MPI_HM_OPEN_MP) $(EXE_MPI_HG_OPEN_MP) $(EXE_MPI_HM_OPEN_MP)
