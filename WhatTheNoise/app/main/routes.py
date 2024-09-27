import subprocess
import os
import shutil
from pathlib import Path
from flask import render_template, request, Blueprint

main = Blueprint('main', __name__)

def run_command(command, error_message):
    """Executes a command and returns its output if successful, otherwise returns an error message"""
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode != 0:
        return None, f"{error_message}: {result.stderr}"
    return result.stdout.strip(), None

def get_devices_info():
    """Gets the ID, name and supported sample rates of single channel audio devices connected to the system"""
    make_command_devices = ['make', '-C', './app/utils', 'list_devices_info']
    run_command_devices = ['./app/utils/list_devices_info']
    
    if not Path('./app/utils/list_devices_info.o').exists():
        output, error = run_command(make_command_devices, "Error executing make for devices")
        if error:
            return None, None, error
    
    output, error = run_command(run_command_devices, "Error executing list_devices_info.o")
    if error:
        return None, None, error
    
    devices = []
    all_sample_rates = []
    
    for line in output.split('\n'):
        if line.strip():
            parts = line.split(', ')
            mic_id = parts[0].split('ID: ')[1]
            name = parts[1].split('Name: ')[1]
            rates = parts[2].split('Rates: ')[1].split()
            devices.append(mic_id + ') ' + name)
            all_sample_rates.append(set(map(int, rates)))
    
    common_sample_rates = set.intersection(*all_sample_rates) if all_sample_rates else set()
    
    return devices, sorted(common_sample_rates), None

@main.route('/')
def index():
    """Renders the index page"""
    return render_template('index.html')

@main.route('/recording_config')
def recording_config():
    """Renders the recording configuration form with the available audio devices and sample rates"""
    devices, sample_rates, error = get_devices_info()

    if error:
        return error, 500

    return render_template('recording_config.html', devices=devices, sample_rates=sample_rates)

from flask import redirect, url_for, flash

@main.route('/start_recording', methods=['POST'])
def start_recording():
    mic1 = request.form.get('mic_1')
    mic2 = request.form.get('mic_2')
    sample_rate = request.form.get('sample_rate')
    threshold = request.form.get('threshold')
    silence_precision = request.form.get('silence_precision')
    use_portaudio = request.form.get('use_portaudio')
    use_alsa = request.form.get('use_alsa')

    if not all([mic1, mic2, sample_rate, threshold, silence_precision]):
        flash('Todos los campos son obligatorios', 'error')
        return redirect(url_for('main.recording_config'))

    if not use_portaudio and not use_alsa:
        flash('Debe seleccionar al menos una opción: Usar PortAudio o Usar ALSA.', 'error')
        return redirect(url_for('main.recording_config'))
    
    if mic1 == mic2:
        flash('Los micrófonos seleccionados deben ser diferentes.', 'error')
        return redirect(url_for('main.recording_config'))

    if use_portaudio:
        mic1 = mic1.split(' ')[0].strip(")")
        mic2 = mic2.split(' ')[0].strip(")")
        
        make_command = ['make', '-C', './app/utils', 'record_PortAudio']
        record_command = ['./app/utils/record_PortAudio', mic1, mic2, sample_rate, threshold, silence_precision]
        
        if not Path('./app/utils/record_PortAudio.o').exists():
            output, error = run_command(make_command, "Error executing make for recording")
            if error:
                return error, 500
            
        record_process = subprocess.Popen(record_command)
        
        with open('./app/utils/record_PortAudio.pid', 'w') as f:
            f.write(str(record_process.pid))
        
    if use_alsa:
        mic1 = mic1.split(' ')[6].strip("()")
        mic2 = mic2.split(' ')[6].strip("()")
        
        make_command = ['make', '-C', './app/utils', 'record_ALSA']
        record_command = ['./app/utils/record_ALSA', mic1, mic2, sample_rate, threshold, silence_precision]
        
        if not Path('./app/utils/record_ALSA.o').exists():
            output, error = run_command(make_command, "Error executing make for recording")
            if error:
                return error, 500
            
        record_process = subprocess.Popen(record_command)
        
        with open('./app/utils/record_ALSA.pid', 'w') as f:
            f.write(str(record_process.pid))
            
    analysis_process = subprocess.Popen(['python', './app/utils/analyzer.py'])
    with open('./app/utils/analyzer.pid', 'w') as f:
        f.write(str(analysis_process.pid))
        
    return redirect(url_for('main.recording_in_progress'))

@main.route('/recording_in_progress')
def recording_in_progress():
    return render_template('recording_in_progress.html')

@main.route('/stop_recording', methods=['POST'])
def stop_recording():
    path = ''
    if Path('./app/utils/record_PortAudio.pid').exists():
        path = './app/utils/record_PortAudio.pid'
    elif Path('./app/utils/record_ALSA.pid').exists():
        path = './app/utils/record_ALSA.pid'
        
    if path:
        try:
            with open(path, 'r') as f:
                pid = int(f.read())
                
            os.system(f'kill {pid}')
            os.remove(path)
            
            recording_dir1 = Path('./samples_threads_Mic1')
            recording_dir2 = Path('./samples_threads_Mic2')
            
            if recording_dir1.exists() and recording_dir1.is_dir():
                shutil.rmtree(recording_dir1)

            if recording_dir2.exists() and recording_dir2.is_dir():
                shutil.rmtree(recording_dir2)
            
        except Exception as e:
            flash(f'Error stopping recording: {str(e)}', 'error')
            
    if Path('./app/utils/analyzer.pid').exists():
        try:
            with open('./app/utils/analyzer.pid', 'r') as f:
                pid = int(f.read())
                
            os.system(f'kill {pid}')
            os.remove('./app/utils/analyzer.pid')
            
        except Exception as e:
            flash(f'Error stopping analysis: {str(e)}', 'error')
    
    return redirect(url_for('main.recording_config'))

@main.route('/finish_recording', methods=['POST'])
def finish_recording():
    path = ''
    if Path('./app/utils/record_PortAudio.pid').exists():
        path = './app/utils/record_PortAudio.pid'
    elif Path('./app/utils/record_ALSA.pid').exists():
        path = './app/utils/record_ALSA.pid'
        
    if path:
        try:
            with open(path, 'r') as f:
                pid = int(f.read())
                
            os.system(f'kill {pid}')
            os.remove(path)
            
            recording_dir1 = Path('./samples_threads_Mic1')
            recording_dir2 = Path('./samples_threads_Mic2')
            
            if recording_dir1.exists() and recording_dir1.is_dir():
                shutil.rmtree(recording_dir1)

            if recording_dir2.exists() and recording_dir2.is_dir():
                shutil.rmtree(recording_dir2)
            
        except Exception as e:
            flash(f'Error stopping recording: {str(e)}', 'error')
            
    if Path('./app/utils/analyzer.pid').exists():
        try:
            with open('./app/utils/analyzer.pid', 'r') as f:
                pid = int(f.read())
                
            os.system(f'kill {pid}')
            os.remove('./app/utils/analyzer.pid')
            
        except Exception as e:
            flash(f'Error stopping analysis: {str(e)}', 'error')
    
    return redirect(url_for('main.recording_results'))

@main.route('/recording_results')
def recording_results():
    return render_template('recording_results.html')

@main.route('/about')
def about():
    return render_template('about.html')