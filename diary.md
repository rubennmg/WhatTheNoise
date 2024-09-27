# Testeo de librerías (ALSA - PortAudio)
## 14/02, miércoles
- Primera reunión
- Instalación y uso de la Raspberry por primera vez

## 15/02, jueves
- Investigación sobre el uso de ALSA
  - Se llega a la conclusión de que es demasiado complejo
- Se decide que se usará la librería Portaudio para capturar y analizar los sonidos
- Primera versión para la captura y reproducción de audio

## 17/02, sábado
- Primer _commit_ en el repositorio oficial
- _Investigación Inicial_
  - Recopilación de información (PortAudio, clasificación de sonidos, TDOA)

# Implementación con PortAudio 
## 21/02, miércoles
- Testeo y pruebas para la ejecución con micrófonos en la Raspberry

## 23/02, viernes
- Finalizada la _Investigación Inicial_
- Implementación y testeo del correcto funcionamiento de la captura y reproducción

## 24/02, sábado
- Subida de ficheros al repositorio oficial

## 25/02, domingo
- Como el audio se almacena en formato crudo, el archivo generado ocupa demasiado (5 segundos de grabación equivalen a 1.8 MB). Por ello se ha modificado `record.c` para que comprima el audio grabado en formato `.mp4` (se hace ejecutando `ffmpeg` con una llamada a `system()`).
- Testear qué pasa en formato comprimido. Probar a decodificar el audio en `play.c` usando las librerías de `ffmpeg`. **[FALTA POR REALIZAR]**
- Pequeña investigación sobre TDOA y mínima implementación. 
- El audio en formato crudo se almacena en la siguiente estructura:
  - `frameIndex (int)`: lleva el seguimiento de la posición actual dentro del array de muestras.
  - `maxFrameIndex (int)`: representa el índice máximo dentro del array de muestras.
  - `recordedSamples (SAMPLE)`: es un puntero a un array de muestras de audio. Las muestras son datos grabados desde un micrófono. Cada elemento del array representa la amplitud de cada muestra en un momento específico en el tiempo.
- ESTUDIAR E INVESTIGAR AÚN:
  - Cómo grabar con los dos micŕofonos simultáneamente. Así se podrían comparar las muestras e instantes de llegada para poder aplicar TDOA
  - Estudio de `ffmpeg` para la decodificación de audio. Comprobar si la codificación se realiza adecuadamente


## 27/02, martes
- Recopilación y recolección de ideas
- **ToDo**:
  - Codificación y decodificación (`ffmpeg`)
  - Grabación con dos micrófonos simultáneos
  - Aplicación de TDOA (fichero de ejemplo)
  - Clasificación de sonidos

## 28/02, miércoles
- Grabación simultánea con dos micrófonos

## 07/03, jueves
- Se revierte el fichero `record.c` para grabar sólo con un micrófono, debido a problemas para implementar correctamente la grabación simultánea con dos
- Fichero `record-2-mics.c` donde se investigará e implementará la grabación con dos micrófonos

## 09/03, sábado
- Uso de dos streams de audio simultáneos, pero con el mismo micrófono

## 10/03, domingo
- Investigación para reconocimiento de errores en la grabación simultánea
- Detección de ruido y errores en los ficheros grabados

## 11/03, lunes
- Pruebas de grabación con diferentes frecuencias de muestreo y número de canales
- Los errores de grabación siguen persistiendo
- Añadidos hilos en paralelo para la grabación en ambos dispositivos (`record-2-mics.c`)

## 12/03, martes
- Pruebas y testeo de diferentes formatos de muestra de audio
- Modificación de la estructura del programa de grabación simultánea (`record-2-mics.c`)

## 15/03, viernes
- Testeo de grabación con UN sólo micrófono
- Detección de errores y problemas con la selección de dispositivos de entrada

## 17/03, domingo
- Implementación de mejoras en ficheros de grabación y reproducción con un sólo micrófono:
  - Selector de formato de muestra. Se establece por defecto `paInt16` (enteros de 16 bits). Esto supone:
    - "Buena" calidad de audio, ficheros de grabación más ligeros. Puede no ser necesaria la compresión de audio
  - Implementación de función de reproducción de audio con UN sólo canal en `play.c`
    - Testeo realizado, aparentemente correcto
- Solución de problemas de formato al seleccionar diferentes dispositivos 
- Implementación y testeo de esta nueva configuración usando los dos micrófonos simultáneamente:
  - Probelmas de sincronización de audio, seguramente por la implementación multithreading
- **ToDo**:
  - Arreglar problema con grabación multithreading
  - Implementación de algoritmo de radiolocalización basado en TDOA
