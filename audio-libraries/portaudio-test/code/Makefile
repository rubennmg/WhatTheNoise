all: portaudio record play tdoa

portaudio: 
	gcc -o portaudio portaudio-test.c -lportaudio

record:
	gcc -o record record.c -lportaudio

play:
	gcc -o play play.c -lportaudio

tdoa: 
	gcc -o tdoa tdoa.c -lportaudio

clean:
	rm -f portaudio record play tdoa