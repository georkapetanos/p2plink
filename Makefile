lrd: lrd.o libjson.a
	gcc -Wall -g lrd.o libjson.a -L /usr/local/lib/ -lssl -lcrypto -o lrd

lrd.o: lrd.c json.h
	gcc -Wall -g -c -Wno-deprecated-declarations lrd.c

lrdtest: lrdtest.o libjson.a
	gcc -Wall -g lrdtest.o libjson.a -o lrdtest

lrdtest.o: lrdtest.c json.h
	gcc -Wall -g -c lrdtest.c

libjson.a: json.c json.h
	gcc -Wall -g -c json.c
	ar rcs libjson.a json.o
	
clean:
	rm lrdtest lrdtest.o libjson.a json.o lrd.o lrd
