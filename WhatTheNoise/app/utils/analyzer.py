import time
import os
import numpy as np
import subprocess
from scipy.optimize import fsolve
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

start_time = time.strftime('%m%d_%H%M')

class FileHandler(FileSystemEventHandler):
    def __init__(self, process_file_callback):
        self.process_file_callback = process_file_callback
        self.processed_files = set()

    def on_closed(self, event):
        if not event.is_directory and event.src_path.endswith('.ts'):
            self.process_file_callback(event.src_path)
            
def get_sound_type(raw_file_path, sample_rate=44100, sample_size=2, threshold=1.2):
    """Determine if the sound is punctual or continuous based on its duration."""
    file_size = os.path.getsize(raw_file_path)
    num_samples = file_size / sample_size
    
    duration = num_samples / sample_rate
    
    if duration < threshold:
        return "Sonido puntual"
    else:
        return "Sonido continuo"

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
    """Calculate the position of the sound source based on TDOA."""
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

def determine_sound_position(positions, distance_between_mics, movement_threshold):
    """Determine movemement and position of the sound source based on the positions over time."""
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

    max_movement = max([abs(positions[i+1][0] - positions[i][0]) for i in range(total_positions - 1)])

    if max_movement > movement_threshold:
        return "En movimiento"
    
    if mic1_percentage > 0.7:  # More than 70% of the positions are near Mic 1
        return "Cercano a Mic 1"
    elif mic2_percentage > 0.7:  # More than 70% of the positions are near Mic 2
        return "Cercano a Mic 2"
    elif center_percentage > 0.7:  # More than 70% of the positions are near the center
        return "Posición central"
    else:
        return "En movimiento"
    
def process_file(file_path):
    """Process a file and calculate the type and sound position based on the timestamps read."""
    mic1_dir = "./samples_threads_Mic1"
    mic2_dir = "./samples_threads_Mic2"
    results_dir = f"./results_{start_time}"
    sounds_dir = os.path.join(results_dir, "sounds")
    os.makedirs(sounds_dir, exist_ok=True)

    file_name = os.path.basename(file_path)
    index = file_name.split('_')[2].split('.')[0]

    if "Mic1" in file_name:
        other_file_path = os.path.join(mic2_dir, f"timestamps_Mic2_{index}.ts")
        raw_file_path = file_path.replace('timestamps_Mic1', 'samples_Mic1').replace('.ts', '.raw')
    else:
        other_file_path = os.path.join(mic1_dir, f"timestamps_Mic1_{index}.ts")
        raw_file_path = file_path.replace('timestamps_Mic2', 'samples_Mic2').replace('.ts', '.raw')

    processed_set_key = (file_path, other_file_path)

    if os.path.exists(other_file_path) and processed_set_key not in file_handler.processed_files:
        with open(file_path, 'r') as f1, open(other_file_path, 'r') as f2:
            data1 = f1.read().strip()
            data2 = f2.read().strip()
            
            timestamps1 = np.array([float(line) for line in data1.split('\n') if line])
            timestamps2 = np.array([float(line) for line in data2.split('\n') if line])
            
            if len(timestamps1) == 0 or len(timestamps2) == 0:
                print("No timestamps found.")
                return

            tdoas = calculate_tdoas_over_time(timestamps1, timestamps2, 10)
            
            if not tdoas:
                print("No TDOAs calculated.")
                return

            positions = [calculate_position(tdoa, 2.15) for tdoa in tdoas]
            
            sound_type = get_sound_type(raw_file_path)
                
            sound_position = determine_sound_position(positions, 2.15, 2.15/100)
            
            log_file_path = os.path.join(results_dir, "results.log")
            with open(log_file_path, 'a') as log_file:
                log_file.write(f"{time.strftime('%Y-%m-%d %H:%M:%S')}, {sound_type}, {sound_position}, {os.path.basename(raw_file_path).replace('.raw', '.mp4')}\n")
                
            sound_id = f"sound_{index}.mp4"
            sound_file_path = os.path.join(sounds_dir, sound_id)
            ffmpeg_command = ['ffmpeg', '-y', '-f', 's16le', '-ar', '44100', '-ac', '1', '-i', raw_file_path, sound_file_path]
            subprocess.run(ffmpeg_command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        # Marcar ambos archivos como procesados
        file_handler.processed_files.add(processed_set_key)
        file_handler.processed_files.add((other_file_path, file_path))
        
if __name__ == "__main__":
    mic1_dir = "./samples_threads_Mic1"
    mic2_dir = "./samples_threads_Mic2"

    file_handler = FileHandler(process_file)
    observer = Observer()

    observer.schedule(file_handler, mic1_dir, recursive=False)
    observer.schedule(file_handler, mic2_dir, recursive=False)
    
    observer.start()

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        observer.stop()
    observer.join()
