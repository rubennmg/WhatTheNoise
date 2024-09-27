import os
import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import fsolve

def read_timestamps(timestamp_file_path):
    """Read timestamps from a binary file and return as a numpy array."""
    timestamps = []
    with open(timestamp_file_path, 'rb') as f:
        while True:
            timestamp_bytes = f.read(8)
            if not timestamp_bytes:
                break
            timestamp = np.frombuffer(timestamp_bytes, dtype=np.double)[0]
            timestamps.append(timestamp)
    return np.array(timestamps)

def write_timestamps_to_text(timestamps, output_file_path):
    """Write timestamps to a text file, one timestamp per line."""
    with open(output_file_path, 'w') as f:
        for timestamp in timestamps:
            f.write(f"{timestamp}\n")

def calculate_average_tdoa(timestamps1, timestamps2):
    """Calculate the average TDOA between two sets of timestamps."""
    min_length = min(len(timestamps1), len(timestamps2))
    tdoas = timestamps1[:min_length] - timestamps2[:min_length]
    return np.mean(tdoas)

def calculate_tdoas_over_time(timestamps1, timestamps2, interval_length):
    """Calculate TDOAs over time using fixed intervals."""
    min_length = min(len(timestamps1), len(timestamps2))
    num_intervals = min_length // interval_length
    tdoas = []
    for i in range(num_intervals):
        start_idx = i * interval_length
        end_idx = start_idx + interval_length
        tdoa = calculate_average_tdoa(timestamps1[start_idx:end_idx], timestamps2[start_idx:end_idx])
        tdoas.append(tdoa)
    return tdoas

def is_sound_moving(tdoas, threshold=0.003):
    """Determine if the sound source is moving based on TDOA changes."""
    changes = 0
    for i in range(1, len(tdoas)):
        if abs(tdoas[i] - tdoas[i-1]) > threshold:
            changes += 1
    return changes > 1

def detect_movement_pattern(tdoas):
    """Detect the movement pattern of the sound source based on TDOA changes."""
    positive_changes = 0
    negative_changes = 0
    for i in range(1, len(tdoas)):
        if tdoas[i] > tdoas[i-1]:
            positive_changes += 1
        elif tdoas[i] < tdoas[i-1]:
            negative_changes += 1
    if positive_changes > negative_changes:
        return "Moving from Mic0 to Mic4"
    elif negative_changes > positive_changes:
        return "Moving from Mic4 to Mic0"
    else:
        return "Indeterminate"

def calculate_distance(tdoa, speed_of_sound=343.0):
    """Calculate the distance to the nearest microphone based on TDOA."""
    return abs(tdoa) * speed_of_sound / 2

def calculate_position(tdoa, d, speed_of_sound=343.0):
    """Calculate the position of the sound source based on TDOA and distance between microphones."""
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

def plot_positions(x, y, distance_between_mics, tdoa):
    """Plot the estimated position of the sound source."""
    fig, ax = plt.subplots()
    ax.scatter(0, 0, color='red', label='Mic0')
    ax.scatter(distance_between_mics, 0, color='blue', label='Mic4')
    ax.scatter(x, y, color='green', label='Sound Source')
    
    ax.set_xlabel('X Position (meters)')
    ax.set_ylabel('Y Position (meters)')
    ax.legend()
    ax.grid(True)
    ax.set_ylim(-0.5, 0.5)
    plt.title(f'Estimated Position of Sound Source: tdoa = {tdoa}')
    plt.show()


def main():
    """Main function to process audio samples and calculate sound source position."""
    mic1_dir = "samples_threads_Mic0"
    mic2_dir = "samples_threads_Mic4"
    sample_rate = 44100 
    interval_length = sample_rate // 2  # 0.5-second intervals
    distance_between_mics = 2.4 # meters
    
    for file1 in sorted(os.listdir(mic1_dir)):
        if file1.endswith(".raw"):
            file2 = file1.replace("Mic0", "Mic4")
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

                    # Write timestamps to text files (debugging)
                    timestamp_txt_filepath1 = timestamp_filepath1.replace(".ts", ".txt")
                    timestamp_txt_filepath2 = timestamp_filepath2.replace(".ts", ".txt")
                    write_timestamps_to_text(timestamps1, timestamp_txt_filepath1)
                    write_timestamps_to_text(timestamps2, timestamp_txt_filepath2)

                    print(f"Timestamps1 size: {timestamps1.size}, Timestamps2 size: {timestamps2.size}")

                    if timestamps1.size != timestamps2.size:
                        print("Warning: Timestamps sizes are not equal, analysis might be inaccurate.")

                    # Calculate TDOAs over time using timestamps and check for movement
                    tdoas_over_time = calculate_tdoas_over_time(timestamps1, timestamps2, interval_length)
                    moving = is_sound_moving(tdoas_over_time)

                    if moving:
                        print("The sound source is moving.")
                        movement_pattern = detect_movement_pattern(tdoas_over_time)
                        print(f"Movement pattern: {movement_pattern}")
                    else:
                        print("The sound source is fixed.")
                        tdoa_timestamps = calculate_average_tdoa(timestamps1, timestamps2)
                        print(f"TDOA from timestamps: {tdoa_timestamps}")
                        if tdoa_timestamps < 0:
                            print("TIMESTAMPS: Sound closer to Mic 0")
                        elif tdoa_timestamps > 0:
                            print("TIMESTAMPS: Sound closer to Mic 4")
                        else:
                            print("TIMESTAMPS: Sound equidistant from both mics")

                        # Calculate distance to the nearest microphone
                        distance = calculate_distance(tdoa_timestamps)
                        print(f"Distance to the nearest microphone: {distance:.4f} meters")

                        position = calculate_position(tdoa_timestamps, distance_between_mics)
                        print(f"Position of the sound source: x = {position[0]:.4f}, y = {position[1]:.4f}")
                    print("-----------------------")

                except Exception as e:
                    print(f"Error processing {timestamp_file1} and {timestamp_file2}: {e}")

if __name__ == "__main__":
    main()
