import numpy as np
import matplotlib.pyplot as plt

SPEED_OF_SOUND = 343.0
DISTANCE_BETWEEN_MIC = 2.1
WINDOW_SIZE = 5
MOVEMENT_THRESHOLD = 0.001

def read_toas(file_path):
    """Lee los TOAs de un archivo .ts y los devuelve como una lista"""
    with open(file_path, 'r') as file:
        lines = file.readlines()
        return [float(line.strip()) for line in lines]

def calculate_tdoas(toa1, toa2):
    """Calcula TDOAs a partir de dos listas de TOAs"""
    min_length = min(len(toa1), len(toa2))
    toa1 = toa1[:min_length]
    toa2 = toa2[:min_length]
    return np.array(toa1) - np.array(toa2)

def determine_sound_position(tdoa):
    """Determina la posición de la fuente de sonido a partir del TDOA"""
    delta_d = tdoa * SPEED_OF_SOUND
    return (DISTANCE_BETWEEN_MIC + delta_d) / 2.0

def moving_average(data, window_size):
    """Calcula la media móvil de una lista de datos"""
    return np.convolve(data, np.ones(window_size)/window_size, mode='valid')

def is_moving(tdoa, threshold):
    """Determina si el sonido está en movimiento basado en los cambios en el TDOA"""
    diff = np.abs(np.diff(tdoa))
    return np.any(diff > threshold)

def categorize_position(position):
    """Categoriza la posición de la fuente de sonido"""
    if position < 0.2 * DISTANCE_BETWEEN_MIC:
        return "Cercana al mic 1"
    elif position > 0.8 * DISTANCE_BETWEEN_MIC:
        return "Cercana al mic 2"
    else:
        return "Central"

toa1 = read_toas('samples_threads_Mic1/timestamps_Mic1_3.ts')
toa2 = read_toas('samples_threads_Mic2/timestamps_Mic2_3.ts')

tdoas = calculate_tdoas(toa1, toa2)

positions = determine_sound_position(tdoas)

filtered_tdoas = moving_average(tdoas, WINDOW_SIZE)
filtered_positions = determine_sound_position(filtered_tdoas)

moving = is_moving(filtered_tdoas, MOVEMENT_THRESHOLD)

if moving:
    print("El sonido está en movimiento.")
else:
    print("El sonido es fijo.")

last_position = positions[-1]
last_filtered_position = filtered_positions[-1]

print(f"Posición de la fuente de sonido (original): {last_position:.2f} metros - {categorize_position(last_position)}")
print(f"Posición de la fuente de sonido (filtrada): {last_filtered_position:.2f} metros - {categorize_position(last_filtered_position)}")

plt.figure(figsize=(12, 6))
plt.plot(tdoas, label='TDOA Original')
plt.plot(np.arange(WINDOW_SIZE-1, len(tdoas)), filtered_tdoas, label='TDOA Filtrado')
plt.xlabel('Índice de Muestra')
plt.ylabel('TDOA (s)')
plt.title('TDOA Original y Filtrado')
plt.legend()
plt.show()

plt.figure(figsize=(12, 6))
plt.plot(positions, label='Posición Original')
plt.plot(np.arange(WINDOW_SIZE-1, len(positions)), filtered_positions, label='Posición Filtrada')
plt.xlabel('Índice de Muestra')
plt.ylabel('Posición de la Fuente de Sonido (m)')
plt.title('Posición de la Fuente de Sonido Original y Filtrada')
plt.legend()
plt.show()
