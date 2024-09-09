import numpy as np
import matplotlib.pyplot as plt
import sys
import os

def plot_occurrences(file_path, output_png):
    # Load binary data
    data = np.fromfile(file_path, dtype=np.uint32)
    A = np.mean(data)
    S = np.std(data)

    # Limit data
    data = np.clip(data, None, A + 3*S)

    # Define the number of milliseconds (10000)
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
    # Check if the input file name is provided
    if len(sys.argv) != 2:
        print("Usage: python script_name.py <input_file>")
        sys.exit(1)

    input_file = sys.argv[1]

    # Create output directory if it doesn't exist
    output_dir = 'images'
    os.makedirs(output_dir, exist_ok=True)

    # Determine the output file name by replacing the .bin extension with .png and adding the directory path
    output_file = os.path.join(output_dir, os.path.splitext(os.path.basename(input_file))[0] + '.png')
    output_file = "images/whatever.png"

    # Call the plotting function
    plot_occurrences(input_file, output_file)
