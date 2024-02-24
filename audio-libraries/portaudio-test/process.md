# PortAudio 
## Primeros pasos
En el fichero `record.c` se lleva a cabo una captura de audio y su posterior reproducción. Se trata de un [programa de ejemplo](https://portaudio.com/docs/v19-doxydocs/paex__record_8c_source.html) de PortAudio. Se activa la escritura a fichero cambiando el flag de `WRITE_TO_FILE` de `0` a `1`. Se procede a probarlo en la Raspberry.

Como se dijo anteriormente, este programa también reproduce el audio capturado. Se modificará pues, separándolo en dos ficheros diferentes: `record.c` y `play.c`. En el primero de ellos se llevará a cabo la captura y escritura de audio en un fichero y en el segundo se leerá y reproducirá audio desde un archivo. Así pues, será posible capturar audio en la Raspberry y reproducirlo en otro dispositivo. El audio se alamcena en formato crudo `.raw`. El siguiente paso será tratar de analizar estos ficheros de audio para localizar la posición de origen del sonido usando TDOA y más de un solo micrófono.

> **_NOTA:_** Para usar PortAudio en la Raspberry es necesario instalar el paquete _portaudio19-dev_ con `sudo apt install portaudio19-dev`.