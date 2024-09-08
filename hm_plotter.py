import numpy as np
import matplotlib.pyplot as plt

'''
This Script creates a HeatMaps (from matrices in *.bin format)
'''

# Read the matrix data from the binary file
with open('map_my_file_10000ms_4.bin', 'rb') as f:
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
plt.title('Heatmap of CSV Data')
plt.xlabel('X')
plt.ylabel('Y')
plt.gca().invert_yaxis()  # Invert the Y-axis to match the typical coordinate system

# Save the heatmap to a PNG file
plt.savefig('whatever.png', dpi=300)
plt.close()
