import pandas as pd
import matplotlib.pyplot as plt
import sys
import os
import pdb
import numpy as np

def load_data(file_name):
    """Load data from a CSV file without headers."""
    try:
        # Load CSV file without headers
        data = pd.read_csv(file_name, header=None)
        # Assign column names manually
        data.columns = ['nodes', 'cpus', 'gpu', 'time']
    except Exception as e:
        print(f"Error loading file: {e}")
        sys.exit(1)
    return data

def filter_data(data, column, value):
    """Filter the data based on column value."""
    return data[data[column] == value]

def sort_data(data, column):
    """Sort the data based on column."""
    return data.sort_values(by=column)

def plot_time_vs_nodes(data, stats_group, group_value, filename):

    df = data[data[stats_group] == group_value]

    # Group by Nodes, CPUS, and GPU to calculate mean and standard deviation
    grouped = df.groupby(['nodes', 'cpus', 'gpu'])['time'].agg(['mean', 'std']).reset_index()

    data_name = filename.removeprefix('summary_')
    data_name = data_name[0].upper() + data_name[1:]

    suffix = ""


    if stats_group == 'cpus':
        # Plot the data with error bars
        plt.figure(figsize=(6, 4))
        suffix = f"gpus_for_{group_value}_cpus"
        plot_title = f'{data_name}: {group_value} CPUs with/without GPU offloading'

        # Plot for GPU=1


        for gpu_count in data['gpu'].unique():
            gpu_group = grouped[grouped['gpu'] == gpu_count]
            if gpu_count == 1:
                connector = "with"
            else:
                connector = "without"
            plt.errorbar(gpu_group['nodes'], gpu_group['mean'], yerr=gpu_group['std'], label=f'{connector} GPU', fmt='-o', capsize=5)

        # Plot for GPU=0

    elif stats_group == 'gpu':
        # Plot the data with error bars
        plt.figure(figsize=(10, 6))
        suffix = f"cpus_for_{group_value}_gpus"
        if group_value == 1:
            connector = 'with'
        else:
            connector = 'without'
        plot_title = f'{data_name}: CPUs {connector} GPU offloading'

        for cpu_count in data['cpus'].unique():
            cpu_group = grouped[grouped['cpus'] == cpu_count]
            plt.errorbar(cpu_group['nodes'], cpu_group['mean'], yerr=cpu_group['std'], label=f'{cpu_count} CPUs', fmt='-o', capsize=5)

    else:
        suffix = f"unknown"
        plot_title = "???"

    
    plt.ylim(0, (int)(data['time'].max()*1.2))

    # Add plot details
    plt.xlabel('Nodes')
    plt.ylabel('Time')
    plt.title(plot_title)
    plt.legend()
    plt.grid(True)

    # Show plot
    plt.savefig(f"images/{filename}_{suffix}.png")

if __name__ == "__main__":

    # Check if the filename argument is provided
    if len(sys.argv) < 2:
        print("Usage: python plot_data.py <csv_file>")
        sys.exit(1)

    # Load CSV file
    file_name = sys.argv[1]
    data = load_data(file_name)

    # Determine the output file name by changing the extension to .png
    output_file = os.path.splitext(file_name)[0]

    
    unique_nodes = data['nodes'].unique()
    unique_cpus = data['cpus'].unique()
    unique_gpus = data['gpu'].unique()

    print(f"Unique nodes: {unique_nodes}")
    print(f"Unique nodes: {unique_cpus}")
    print(f"Unique nodes: {unique_gpus}")

    for cpu_count in unique_cpus:
        plot_time_vs_nodes(data, 'cpus', cpu_count, output_file)  

    for gpu_count in unique_gpus:
        plot_time_vs_nodes(data, 'gpu', gpu_count, output_file)  

