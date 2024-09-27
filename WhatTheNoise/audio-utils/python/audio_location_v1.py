##################################
######## audio_location_v1.py #########
##################################
# 
# This script estimates the position of a sound source
# calculating the Time Difference of Arrival (TDOA) between two microphones.
# TDOA is calculated using the Generalized Cross-Correlation with Phase Transform (GCC-PHAT) method
# between the audio signals captured by both microphones.
#
# This script was used for testing purposes.
#
# ~ Author: rubennmg
#  
#################################

import os
import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import correlate

### Constants ###
sample_rate = 44100  # Sample rate of the audio data
speed_of_sound = 343  # Speed of sound in m/s (approximate)

def read_raw_file(filename, sample_format=np.int16):
    """Read raw audio data from a file."""
    with open(filename, 'rb') as f:
        raw_data = f.read()
    data = np.frombuffer(raw_data, dtype=sample_format)
    return data

def gcc_phat_interp(sig, refsig, fs=1):
    """Compute the GCC-PHAT with interpolation for better time resolution."""
    # Make sure the signals are the same length
    n = sig.shape[0] + refsig.shape[0]
    
    # FFT of input signals
    SIG = np.fft.rfft(sig, n=n)
    REFSIG = np.fft.rfft(refsig, n=n)
    
    # Cross-spectral density
    R = SIG * np.conj(REFSIG)
    
    # Phase transform
    R = R / np.abs(R)
    
    # Inverse FFT to get correlation
    corr = np.fft.irfft(R, n=(2*n-1))
    
    max_shift = int(n/2)
    corr = np.concatenate((corr[-max_shift:], corr[:max_shift+1]))
    
    # Find the max correlation index with sub-sample interpolation
    max_index = np.argmax(np.abs(corr))
    if max_index == 0 or max_index == len(corr) - 1:
        tdoa = max_index - max_shift
    else:
        # Perform parabolic interpolation around max_index
        x = np.arange(max_index - 1, max_index + 2)
        y = corr[x]
        tdoa = np.sum(x * y) / np.sum(y) - max_shift
    
    tdoa_sec = tdoa / float(fs)
    
    return tdoa_sec, corr

def determine_position(tdoa):
    """Determines the position of the sound source in a simple way using TDOA."""
    if np.abs(tdoa) > 0.00001: # A very small TDOA can be considered as a central position
        return "Central"
    elif tdoa < 0: # Negative TDOA means the sound is closer to Mic 1
        return "Near Mic 1"
    else: # Positive TDOA means the sound is closer to Mic 2
        return "Near Mic 2"

def main():
    mic1_dir = "samples_threads_Mic1"
    mic2_dir = "samples_threads_Mic2"

    for file1 in sorted(os.listdir(mic1_dir)):
        if file1.endswith(".raw"):
            file2 = file1.replace("Mic1", "Mic2")
            filepath1 = os.path.join(mic1_dir, file1)
            filepath2 = os.path.join(mic2_dir, file2)
            
            if os.path.exists(filepath2):
                data1 = read_raw_file(filepath1)
                data2 = read_raw_file(filepath2)
                
                tdoa, corr = gcc_phat_interp(data1, data2, sample_rate)

                position = determine_position(tdoa)
                
                print(f"TDOA for {file1} and {file2}: {tdoa:.6f} seconds (Adjusted: {tdoa:.6f} seconds)")
                print(f"Position: {position}")
                print("-----------------------")

if __name__ == "__main__":
    main()
