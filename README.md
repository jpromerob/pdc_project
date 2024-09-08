# pdc_project

Processing event-based data.

### Creating Histograms in C : Matrices in *.bin format 
```srun -N 1 -A edu24.summer -t 00:10:00 -p main ./hg_open_mp.exe --file events/<event_file_name>.bin --threads <# threads>```

### Creating Heatmaps in C : Matrices in *.bin format 
```srun -N 1 -A edu24.summer -t 00:10:00 -p main ./hm_open_mp.exe --file events/<event_file_name>.bin --threads <# threads>```
