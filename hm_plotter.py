import numpy as np
import matplotlib.pyplot as plt
import sys
import os

'''
This Script creates a HeatMaps (from matrices in *.bin format)
'''

# Check if the input file name is provided
if len(sys.argv) != 2:
    print("Usage: python hm_plotter.py <input_file>")
    sys.exit(1)

input_file = sys.argv[1]

# Create output directory if it doesn't exist
output_dir = 'images'
os.makedirs(output_dir, exist_ok=True)

# Extract the base name of the input file (without the path) and replace the .bin extension with .png
output_file = os.path.join(output_dir, os.path.splitext(os.path.basename(input_file))[0] + '.png')

# Read the matrix data from the binary file
with open(input_file, 'rb') as f:
    matrix = np.fromfile(f, dtype=np.uint32).reshape((640, 480))

# Calculate mean and standard deviation
mean = np.mean(matrix)
std = np.std(matrix)

# Define lower and upper limits
lower_limit = mean - std
upper_limit = mean + std

# Apply limits to the matrix
matrix = np.clip(matrix, lower_limit, upper_limit)

# Plot the heatmap
plt.imshow(matrix.T, cmap='hot', interpolation='nearest')
plt.colorbar(label='Frequency')
plt.title('Heatmap')
plt.xlabel('X')
plt.ylabel('Y')
plt.gca().invert_yaxis()  # Invert the Y-axis to match the typical coordinate system

# Save the heatmap to a PNG file
plt.savefig(output_file, dpi=300)
plt.close()
