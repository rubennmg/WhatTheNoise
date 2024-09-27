# Script para manejar y clasificar audio leído desde archivos .raw transformándolo en señales.
# Grafica su espectrograma, amplitud y transformada de Fourier.
# El intento de clasificación de audio de este modo resulta un fracaso.
#
# ~ rubennmg

import numpy as np
import scipy.signal as signal
import matplotlib.pyplot as plt

def load_raw_file(filename):
    return np.fromfile(filename, dtype=np.int16) # En este caso es int16 

def plot_spectrogram(audio_signal, sample_rate):
    f, t, Sxx = signal.spectrogram(audio_signal, fs=sample_rate)
    plt.pcolormesh(t, f, 10 * np.log10(Sxx))
    plt.ylabel('Frequency [Hz]')
    plt.xlabel('Time [sec]')
    plt.colorbar(label='Intensity [dB]')
    plt.title('Spectrogram')
    plt.show()

def plot_amplitude(audio_signal, sample_rate):
    times = np.arange(len(audio_signal)) / sample_rate
    plt.figure(figsize=(12, 6))
    plt.plot(times, audio_signal)
    plt.xlabel('Time [sec]')
    plt.ylabel('Amplitude')
    plt.title('Amplitude vs Time')
    plt.show()

def plot_fourier_transform(audio_signal, sample_rate):
    n = len(audio_signal)
    f = np.fft.fftfreq(n, 1 / sample_rate)
    fft_spectrum = np.fft.fft(audio_signal)
    magnitude = np.abs(fft_spectrum)[:n//2]
    f = f[:n//2]
    
    plt.figure(figsize=(12, 6))
    plt.plot(f, magnitude)
    plt.xlabel('Frequency [Hz]')
    plt.ylabel('Magnitude')
    plt.title('Fourier Transform')
    plt.show()

def is_continuous_fixed(audio_signal, sample_rate, threshold=0.1):
    f, t, Sxx = signal.spectrogram(audio_signal, fs=sample_rate)
    spectrum_variance = np.var(Sxx, axis=1)
    
    if np.all(spectrum_variance < threshold):
        return True
    return False 

def is_continuous_moving(audio_signal, sample_rate, threshold=0.1):
    f, t, Sxx = signal.spectrogram(audio_signal, fs=sample_rate)
    spectrum_variance = np.var(Sxx, axis=1)
    
    if np.any(spectrum_variance > threshold) and np.mean(spectrum_variance) < 10 * threshold:
        return True
    return False

def is_punctual(audio_signal, sample_rate, peak_threshold=1000):
    peaks, _ = signal.find_peaks(audio_signal, height=peak_threshold)
    if len(peaks) > 0:
        return True
    return False

def classify_sound(audio_signal, sample_rate):
    if is_continuous_fixed(audio_signal, sample_rate):
        return "Continuous Fixed"
    elif is_continuous_moving(audio_signal, sample_rate):
        return "Continuous Moving"
    elif is_punctual(audio_signal, sample_rate):
        return "Punctual"
    else:
        return "Other"

def main():
    filename = 'output.raw'
    sample_rate = 44100
    audio_signal = load_raw_file(filename)

    audio_signal = audio_signal / np.max(np.abs(audio_signal))

    plot_spectrogram(audio_signal, sample_rate)
    plot_amplitude(audio_signal, sample_rate)
    plot_fourier_transform(audio_signal, sample_rate)

    sound_type = classify_sound(audio_signal, sample_rate)
    print("Sound type:", sound_type)

if __name__ == '__main__':
    main()
