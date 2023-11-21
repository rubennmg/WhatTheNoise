# WhatTheNoise

Sistema para escuchar eventos dentro de un sistema físico (máquina, dispositivo, etc) usando dos micrófonos, localizando el origen y tipo del sonido y registrándolo.

## Fases

### Fase 1: Localización y registro de sonidos

Implementación de un sistema que sea capaz de detectar sonidos cercanos mediante dos micrófonos y localizar el origen (de forma aproximada) mediante técnicas TDOA(Time Difference Of Arrival). Se dispondrá de una Rapsberry Pi con dos micrófonos USB.

El sistema deberá permitir configuración de umbral de sonido y calibración. Con lo que tendrá un pequeño front-end web para configurarlo.

La precisión debe ser prioridad en este proyecto, pues la distancia de los micrófonos al sonido podrá estar entre 0,5m y 4m (aproximadamente):

Al menos, debe detectar y registrar tres tipos de sonidos:

- **Continuos fijos** (tipo motores, ventildores, etc) donde se registra sólo una muestra de X segundos y su posición relativa.

- **Continuos en movimiento** (cuando el mismo sonido va moviéndose entre dos puntos) donde sólo se registra una muestra y posición *en movimiento*.

- **Puntuales** (tipo golpe, chasquido, etc) donde se registra todo el sonido y su posición relativa.

### Fase 2: Creación de un dataset para la clasificación de los sonidos

Se creará una pequeña aplicación (web, cloud o de escritorio) que cruce los datos de los sonidos antes registrados, con los eventos y estados registrados de la máquina/sistema en un fichero de log aparte dado, donde también hay un registro de tiempo. Se buscará una relación entre los sonidos y esos eventos y estados y se clasificarán en base a esta.

El proyecto debe utilizar el estado del arte actual.
