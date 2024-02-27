all: portaudio record play play-mp4 tdoa

portaudio: 
	gcc -o portaudio portaudio-test.c -lportaudio

record:
	gcc -o record record.c -lportaudio

play:
	gcc -o play play.c -lportaudio

play-mp4:
	gcc -o play-mp4 play-mp4.c -lportaudio -lavformat -lavcodec -lavutil -I/usr/include/ffmpeg

tdoa: 
	gcc -o tdoa tdoa.c -lportaudio

clean:
	rm -f portaudio record play play-mp4 tdoa