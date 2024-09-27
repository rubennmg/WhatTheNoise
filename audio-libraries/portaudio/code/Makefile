all: portaudio record play play-mp4 tdoa devices

portaudio: 
	gcc -o portaudio.o portaudio-test.c -lportaudio

devices:
	gcc -o devices.o list-devices.c -lportaudio

record:
	gcc -o record.o record.c -lportaudio

record-v2:
	gcc -o record-v2.o record-v2.c -lportaudio

record-2:
	gcc -o record-2.o record-2-mics.c -lportaudio

record-2-tdoa:
	gcc -o record-2-tdoa.o record-2-mics-tdoa.c -lportaudio -lm

play:
	gcc -o play.o play.c -lportaudio -g

play-v2:
	gcc -o play-v2.o play-v2.c -lportaudio -g

play-mp4:
	gcc -o play-mp4.o play-mp4.c -lportaudio -lavformat -lavcodec -lavutil -I/usr/include/ffmpeg

tdoa: 
	gcc -o tdoa.o tdoa.c -lportaudio

record-2-test:
	gcc -o record-2-mics-test.o record-2-mics-test.c -lportaudio -lm

clean:
	rm -f portaudio.o record.o record-2.o play.o play-mp4.o tdoa.o devices.o