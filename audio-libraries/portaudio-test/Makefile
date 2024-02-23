all: portaudio

portaudio: 
	gcc -o portaudio portaudio-test.c -lportaudio

record:
	gcc -o record record.c -lportaudio

clean:
	rm -f portaudio