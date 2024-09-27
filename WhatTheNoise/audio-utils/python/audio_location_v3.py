##################################
######## audio_location_v3.py #########
##################################
#
# File used for testing and debugging purposes.
# 
##################################

import os
import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import fsolve

def read_timestamps(timestamp_file_path):
    """Read timestamps from a file and return as a numpy array."""
    timestamps = []
    with open(timestamp_file_path, 'r') as f:
        for line in f:
            timestamp = float(line.strip())
            timestamps.append(timestamp)
    return np.array(timestamps)

def calculate_tdoas_over_time(timestamps1, timestamps2, interval_length):
    """Calculate TDOAs over time using fixed intervals."""
    min_length = min(len(timestamps1), len(timestamps2))
    num_intervals = int(min_length / interval_length)
    tdoas = []
    for i in range(num_intervals):
        pos = int(i * interval_length)
        tdoa = timestamps1[pos] - timestamps2[pos]
        tdoas.append(tdoa)
    return tdoas

def calculate_position(tdoa, d, speed_of_sound=343.0):
    """Calculate the position of the sound source based on TDOA and assuming that it is between both microphones"""
    delta_d = tdoa * speed_of_sound
    
    # Define the system of equations
    def equations(vars):
        x, y = vars
        eq1 = np.sqrt(x**2 + y**2) - np.sqrt((x - d)**2 + y**2) - delta_d
        eq2 = x - (d / 2)  # Assume the source is directly between the mics on average
        return [eq1, eq2]
    
    # Initial guess for fsolve
    initial_guess = [d / 2, 0]
    
    # Solve the system of equations
    x, y = fsolve(equations, initial_guess)
    return x, y

def determine_sound_movement(positions, distance_between_mics, movement_threshold=0.23):
    """Determine if the sound source is moving based on the positions over time."""
    mic1_range = (0, distance_between_mics / 3)
    center_range = (distance_between_mics / 3, 2 * distance_between_mics / 3)
    mic2_range = (2 * distance_between_mics / 3, distance_between_mics)
    
    near_mic1 = 0
    near_mic2 = 0
    near_center = 0
    
    for x, y in positions:
        if x < 0:
            near_mic1 += 1
        elif x > distance_between_mics:
            near_mic2 += 1
        elif mic1_range[0] <= x <= mic1_range[1]:
            near_mic1 += 1
        elif center_range[0] <= x <= center_range[1]:
            near_center += 1
        elif mic2_range[0] <= x <= mic2_range[1]:
            near_mic2 += 1
    
    total_positions = len(positions)
    mic1_percentage = near_mic1 / total_positions
    mic2_percentage = near_mic2 / total_positions
    center_percentage = near_center / total_positions
    
    print(f"Mic1 percentage: {mic1_percentage}")
    print(f"Mic2 percentage: {mic2_percentage}")
    print(f"Center percentage: {center_percentage}")

    initial_position = positions[0][0]
    final_position = positions[-1][0]
    max_movement = max([abs(positions[i+1][0] - positions[i][0]) for i in range(total_positions - 1)])
    
    print(f"Initial position: {initial_position}, Final position: {final_position}")
    print(f"Max movement between intervals: {max_movement}")

    if max_movement > movement_threshold:
        return "Moving"
    
    if mic1_percentage > 0.7:  # More than 70% of the positions are near Mic 1
        return "Fixed near Mic 1"
    elif mic2_percentage > 0.7:  # More than 70% of the positions are near Mic 2
        return "Fixed near Mic 2"
    elif center_percentage > 0.7:  # More than 70% of the positions are near the center
        return "Fixed near center"
    else:
        return "Moving"

def plot_positions(positions, distance_between_mics, title):
    """Plot the positions of the sound source over time."""
    fig, ax = plt.subplots()
    ax.scatter(0, 0, color='red', label='Mic1')
    ax.scatter(distance_between_mics, 0, color='blue', label='Mic2')
    for pos in positions:
        ax.scatter(pos[0], pos[1], color='green')
    
    ax.set_xlabel('X Position (meters)')
    ax.set_ylabel('Y Position (meters)')
    ax.legend()
    ax.grid(True)
    ax.set_ylim(-0.5, 0.5)
    plt.title(title)
    plt.show()

def main():
    mic1_dir = "samples_threads_Mic1"
    mic2_dir = "samples_threads_Mic2"
    interval_length = 10
    distance_between_mics = 2.2
    positions = []

    for file1 in sorted(os.listdir(mic1_dir)):
        if file1.endswith(".raw"):
            file2 = file1.replace("Mic1", "Mic2")
            timestamp_file1 = file1.replace("samples_", "timestamps_").replace(".raw", ".ts")
            timestamp_file2 = file2.replace("samples_", "timestamps_").replace(".raw", ".ts")

            print(f"Timestamp file 1: {timestamp_file1}, Timestamp file 2: {timestamp_file2}")

            filepath1 = os.path.join(mic1_dir, file1)
            filepath2 = os.path.join(mic2_dir, file2)
            timestamp_filepath1 = os.path.join(mic1_dir, timestamp_file1)
            timestamp_filepath2 = os.path.join(mic2_dir, timestamp_file2)

            if (os.path.exists(filepath1) and os.path.exists(filepath2) and 
                os.path.exists(timestamp_filepath1) and os.path.exists(timestamp_filepath2)):
                try:
                    timestamps1 = read_timestamps(timestamp_filepath1)
                    timestamps2 = read_timestamps(timestamp_filepath2)

                    print(f"Timestamps1 size: {timestamps1.size}, Timestamps2 size: {timestamps2.size}")

                    if timestamps1.size != timestamps2.size:
                        print("Warning: Timestamps sizes are not equal, analysis might be inaccurate.")

                    tdoas_over_time = calculate_tdoas_over_time(timestamps1, timestamps2, interval_length)
                    
                    for tdoa in tdoas_over_time:
                        print(f"TDOA: {tdoa:.9f}")
                        position = calculate_position(tdoa, distance_between_mics)
                        positions.append(position)
                        print(f"Position: x = {position[0]:.4f}, y = {position[1]:.4f}")
                    
                    sound_movement = determine_sound_movement(positions, distance_between_mics)
                    print(f"Sound Position: {sound_movement}")

                    plot_positions(positions, distance_between_mics, f'Estimated Position of Sound Source for: {filepath1} - Movement = {sound_movement}')
                    positions.clear()
                    tdoas_over_time.clear()
                    
                    print("-----------------------")

                except Exception as e:
                    print(f"Error processing {timestamp_file1} and {timestamp_file2}: {e}")

if __name__ == "__main__":
    main()
