CFLAGS=-Wall -ansi -lm -g

sndconv: cs229.o sndconv.o aiff.o
	$(CC) $(CFLAGS) cs229.o sndconv.o aiff.o -o sndconv

sndconv.o: sndconv.c cs229.h utils.h aiff.h
	$(CC) $(CFLAGS) -c sndconv.c

sndinfo: cs229.o sndinfo.o aiff.o
	$(CC) $(CFLAGS) cs229.o sndinfo.o aiff.o -o sndinfo

sndinfo.o: sndinfo.c cs229.h utils.h aiff.h
	$(CC) $(CFLAGS) -c sndinfo.c

aiff.o: aiff.c aiff.h sound.h utils.h
	$(CC) $(CFLAGS) -c aiff.c

cs229.o: cs229.c cs229.h sound.h
	$(CC) $(CFLAGS) -c cs229.c

clean:
	rm -f *.o sndinfo sndconv
