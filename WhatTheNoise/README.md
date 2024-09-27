# WhatTheNoise 🎙️
WhatTheNoise es un proyecto cuyo objetivo es el registro de eventos basado en la clasificación y análisis de los sonidos captados por dos micrófonos. 

## Entorno y configuración
### Librerías de C
-> `ALSA`

-> `PortAudio`

-> `FFmpeg`
### Instalación:
#### Debian (Ubuntu, Raspberry Pi OS)
```sh
sudo apt install libasound2-dev portaudio19-dev ffmpeg
```
#### RedHat (Fedora)
```sh
sudo dnf install alsa-lib-devel portaudio portaudio-devel ffmpeg
```

---

### Librerías de python
-> `Flask` 

-> `Numpy` 

-> `Scipy` 

-> `Watchdog` 


> **_NOTA:_** Se recomienda el uso de un entorno virtual para la instalación y uso de librerías de Python y lanzamiento del servidor.
#### Pasos a seguir para la creación de un entorno virtual:
Creación de un nuevo entorno virtual en el directorio raíz del proyecto:
```sh
python3 -m venv .venv
```
Activación del entorno virtual:
```sh
source .venv/bin/activate
```
Desactivación del entorno virtual:
```sh 
deactivate
```
#### Instalación de librerías
Una vez el entorno virtual esté activo, ejecutar:
```sh
pip install flask numpy scipy watchdog
```
---

### Lanzamiento del servidor
#### En modo de desarrollo
Ejecutar desde el directorio raíz del proyecto y con el entorno virtual activado (si es que se ha elegido esta opción):
```sh
python3 run.py
```

#### Con Docker
Primero debe crearse un Dockerfile:
```docker
FROM python:3.8-slim 

WORKDIR /app 

COPY requirements.txt requirements.txt

RUN pip install -r requirements.txt 

COPY . .

EXPOSE 5000

CMD ["gunicorn", "--bind", "0.0.0.0:5000", "wsgi:app"]
```

Tras esto, debe de crearse un fichero (requirements.txt) especificando las librerías de Python necesarias:
```text
Flask
Numpy
Scipy
Watchdog
Gunicorn
```
Creación de la imagen de Docker:
```bash
docker build -t app_name .
```
Ejecución de la imagen:
```bash
docker run -p 5000:5000 app_name
```

## Estrucura de directorios
```plaintext
WhatTheNoise/
├── app
│   ├── __init__.py
│   ├── main/
│   │   ├── __init__.py
│   │   └── __pycache__
│   │       ├── __init__.cpython-312.pyc
│   │       ├── routes.cpython-312.pyc
│   │       └── views.cpython-312.pyc
│   ├── __pycache__
│   │   └── __init__.cpython-312.pyc
│   ├── static/
│   │   ├── css
│   │   │   └── styles.css
│   │   ├── images
│   │   │   ├── mic.svg
│   │   │   └── recording.svg
│   │   └── js
│   │       └── scripts.js
│   ├── templates/
│   │   ├── about.html
│   │   ├── base.html
│   │   ├── index.html
│   │   ├── recording_config.html
│   │   ├── recording_in_progress.html
│   │   └── recording_results.html
│   └── utils/
│       ├── analyzer.py
│       ├── list_devices_info.c
│       ├── list_devices_info.o
│       ├── makefile
│       ├── record_ALSA.c
│       ├── record_ALSA.o
│       ├── record.c
│       ├── record_PortAudio.c
│       └── record_PortAudio.o
├── audio-utils/
│   ├── C/
│   │   ├── alsa_check_hw_timestamps.c
│   │   ├── alsa_htimestamp_test.c
│   │   ├── list_devices.c
│   │   ├── list_sample_rates.c
│   │   ├── makefile
│   │   ├── record_v0.c
│   │   ├── record_v1.c
│   │   ├── record_v2.c
│   │   ├── record_v3.c
│   │   ├── record_v4.c
│   │   └── record_v5.c
│   └── python/
│       ├── audio_location_v1.py
│       ├── audio_location_v2.py
│       ├── audio_location_v3.py
│       ├── audio_location_v4.py
│       ├── audio_location_v5.py
│       └── handle_audio.py
├── config.py
├── README.md
└── run.py
```

- **WhatTheNoise/**: Directorio raíz del proyecto
  - **app/**: Contiene la aplicación principal y sus módulos
    - **__init__.py**: Archivo de inicialización
    - **main/**: Módulo principal
      - **__init__.py**: Archivo de inicialización
      - **routes.py**: Define las rutas de la aplicación
    - **static/**: Archivos estáticos 
      - **css/**: Directorio de hojas de estilo
        - **styles.css**: Estilos de la aplicación
      - **images/**: Directorio de imágenes
        - **mic.svg**
        - **recording.svg**
      - **js/**: Directorio de archivos JavaScript
        - **scripts.js**: Scripts JavaScript de la aplicación
    - **templates/**: Plantillas HTML de la aplicación
      - **about.html**: Vista "Acerca de"
      - **base.html**: Plantilla base
      - **index.html**: Página principal
      - **recording_config.html**: Vista del formulario de la aplicación
      - **recording_in_progress.html**: Vista de grabación en curso
      - **recording_results.html**: Vista final del proceso de grabación
    - **utils/**: Utilidades y herramientas de la aplicación
      - **analyzer.py**: Analiza y clasifica los sonidos captados
      - **list_devices_info.c**: Lista información a mostrar en el formulario de configuración sobre los dispositivos de audio disponibles
      - **record_ALSA.c**: Versión de programa de grabación utilizando ALSA y timestamps de hardware
      - **record_PortAudio**: Versión de programa de grabación utilizando PortAudio
      - **makefile**: Compilador de programas
  - **audio_utils/**: Directorio con diferentes versiones de archivos de utilidad para el desarrollo del proyecto.
    - **C/**: Subdirectorio con programas de utilidad escritos en C
      - **alsa_check_hw_timestamps.c**: Comprueba si el dispositivo de audio seleccionado permite el usdo de timestamos de hardware.
      - **alsa_htimestamp_test.c**: Configura un dispositivo para utilizar timestamps de hardware
      - **list_devices.c**: Lista todos los dispositivos de audio del sistema.
      - **list_sample_rates.c**: Lista las frecuencias de muestreo compatibles con cada dispositivo del sistema
      - **record_v0.c**: Primera versión del programa de grabación
      - **record_v1.c**: Segunda versión del programa de grabación
      - **record_v2.c**: Tercera versión del programa de grabación
      - **record_v3.c**: Cuarta versión del programa de grabación
      - **record_v4.c**: Quinta versión del programa de grabación
      - **record_v5.c**: Sexta versión del programa de grabación
      - **makefile**: Compilador de programas
    - **Python/**: Subdirectorio con programas de utilidad escritos en Python
      - **audio_location_v1.py**: Primera versión del programa de localización de audio
      - **audio_location_v2.py**: Segunda versión del programa de localización de audio
      - **audio_location_v3.py**: Tercera versión del programa de localización de audio
      - **audio_location_v4.py**: Cuarta versión del programa de localización de audio  
      - **audio_location_v5.py**: Quinta versión del programa de localización de audio 
      - **handle_audio.py**: Clasificación y localización de sonidos mediante análisis de señales
  - **config.py**: Archivo de configuración de la aplicación
  - **run.py**: Script para lanzar el proyecto
  - **README.md**: Documentación del proyecto