all: listing listing2 portaudio

listing: 
	gcc -o listing listing.c -lasound
# -lasound is the library for ALSA

listing2: 
	gcc -o listing2 listing2.c -lasound 
# gdb ./listing2 
# gdb is the GNU debugger
# option -g is for valgrind debugging

portaudio: 
	gcc -o portaudio portaudio-test.c -lportaudio

clean:
	rm -f listing listing2 portaudio