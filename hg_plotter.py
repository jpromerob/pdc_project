import numpy as np
import matplotlib.pyplot as plt

def plot_occurrences(file_path, output_png):
    # Load binary data
    data = np.fromfile(file_path, dtype=np.uint32)
    
    # Define the number of milliseconds (1000)
    num_milliseconds = 10000
    
    if len(data) != num_milliseconds:
        raise ValueError(f"Expected {num_milliseconds} elements, but found {len(data)}")

    # Plotting
    plt.figure(figsize=(12, 6))
    plt.plot(data/1000, marker='o', linestyle='-', color='b')
    plt.xlabel('t [ms]')
    plt.ylabel('Activity [Mev/s]')
    plt.grid(True)
    
    # Save the plot as a PNG file
    plt.savefig(output_png, format='png')
    print(f"Plot saved to {output_png}")

if __name__ == "__main__":
    plot_occurrences('occurrences.bin', 'occurrences_plot.png')
