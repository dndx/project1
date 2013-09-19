CFLAGS=-Wall -ansi -lm -g

sndinfo: cs229.o sndinfo.o aiff.o
	$(CC) $(CFLAGS) cs229.o sndinfo.o aiff.o -o sndinfo

sndinfo.o: sndinfo.c cs229.h utils.h
	$(CC) $(CFLAGS) -c sndinfo.c

aiff.o: aiff.c aiff.h sound.h utils.h
	$(CC) $(CFLAGS) -c aiff.c

cs229.o: cs229.c cs229.h sound.h
	$(CC) $(CFLAGS) -c cs229.c

clean:
	rm -f *.o
