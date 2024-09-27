CC = gcc
CFLAGS = -Wall -Wextra
LIB_ALSA = -lasound
LIB_PORTAUDIO = -lportaudio
LIB_PTHREAD = -lpthread

TARGETS = alsa_htimestamp_test alsa_check_hw_timestamps list_devices list_sample_rates \
          record_v1 record_v2 record_v3 record_v4 record_v5

all: $(TARGETS)

alsa_htimestamp_test: alsa_htimestamp_test.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIB_ALSA)

alsa_check_hw_timestamps: alsa_check_hw_timestamps.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIB_ALSA)

list_devices: list_devices.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIB_PORTAUDIO)

list_sample_rates: list_sample_rates.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIB_PORTAUDIO)

record_v1: record_v1.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIB_PORTAUDIO) $(LIB_PTHREAD)

record_v2: record_v2.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIB_PORTAUDIO) $(LIB_PTHREAD)

record_v3: record_v3.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIB_ALSA) $(LIB_PTHREAD)

record_v4: record_v4.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIB_ALSA) $(LIB_PTHREAD)

record_v5: record_v5.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIB_ALSA) $(LIB_PTHREAD)

.PHONY: clean
clean:
	rm -f $(TARGETS) *.o
