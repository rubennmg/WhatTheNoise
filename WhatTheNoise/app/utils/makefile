CC = gcc
CFLAGS = -Wall -Wextra
LIBS_PORTAUDIO = -lportaudio
LIBS_ALSA = -lasound
LIBS_PTHREAD = -lpthread

TARGETS = list_devices_info record_ALSA record_PortAudio

all: $(TARGETS)

list_devices_info: list_devices_info.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS_PORTAUDIO)

record_ALSA: record_ALSA.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS_ALSA) $(LIBS_PTHREAD)

record_PortAudio: record_PortAudio.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS_PORTAUDIO) $(LIBS_PTHREAD)

.PHONY: clean
clean:
	rm -f $(TARGETS) *.o
