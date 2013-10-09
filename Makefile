CFLAGS=-Wall -ansi -lm -g -DNDEBUG

all: sndconv sndinfo sndcat sndcut sndshow

sndshow: cs229.o sndshow.o aiff.o utils.o
	$(CC) $(CFLAGS) cs229.o sndshow.o aiff.o utils.o -o sndshow

sndcut: cs229.o sndcut.o aiff.o utils.o
	$(CC) $(CFLAGS) cs229.o sndcut.o aiff.o utils.o -o sndcut

sndcat: cs229.o sndcat.o aiff.o utils.o
	$(CC) $(CFLAGS) cs229.o sndcat.o aiff.o utils.o -o sndcat

sndconv: cs229.o sndconv.o aiff.o utils.o
	$(CC) $(CFLAGS) cs229.o sndconv.o aiff.o utils.o -o sndconv

sndinfo: cs229.o sndinfo.o aiff.o utils.o
	$(CC) $(CFLAGS) cs229.o sndinfo.o aiff.o utils.o -o sndinfo

sndshow.o: sndshow.c cs229.h utils.h aiff.h
	$(CC) $(CFLAGS) -c sndshow.c

sndcut.o: sndcut.c cs229.h utils.h aiff.h
	$(CC) $(CFLAGS) -c sndcut.c

sndcat.o: sndcat.c cs229.h utils.h aiff.h
	$(CC) $(CFLAGS) -c sndcat.c

sndconv.o: sndconv.c cs229.h utils.h aiff.h
	$(CC) $(CFLAGS) -c sndconv.c

sndinfo.o: sndinfo.c cs229.h utils.h aiff.h
	$(CC) $(CFLAGS) -c sndinfo.c

aiff.o: aiff.c aiff.h sound.h utils.h
	$(CC) $(CFLAGS) -c aiff.c

cs229.o: cs229.c cs229.h sound.h utils.h
	$(CC) $(CFLAGS) -c cs229.c

utils.o: utils.h
	$(CC) $(CFLAGS) -c utils.c

clean:
	rm -f *.o sndinfo sndconv

