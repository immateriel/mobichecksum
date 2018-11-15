all:
	gcc mobichecksum.c -o mobichecksum -lmobi -lcrypto

clean:
	rm mobichecksum