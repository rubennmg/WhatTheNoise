all: portaudio

portaudio: 
	gcc -o portaudio portaudio-test.c -lportaudio

record:
	gcc -o record record.c -lportaudio

play:
	gcc -o play play.c -lportaudio

clean:
	rm -f portaudio