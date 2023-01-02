CC = gcc

CFLAGS = -Wall -g

LDFLAGS =  -lmosquitto

lrd: lrd.o lib_lrdshared.a test python_bind
	$(CC) $(CFLAGS) lrd.o lib_lrdshared.a -o lrd -lmosquitto

lrd_shared.o: lrd_shared.c lrd_shared.h
	$(CC) $(CFLAGS) -c lrd_shared.c -o lrd_shared.o

json.o: json.c json.h
	$(CC) $(CFLAGS) -c json.c -o json.o
	
mqtt.o: mqtt.c mqtt.h
	$(CC) $(CFLAGS) -c mqtt.c -o mqtt.o -lmosquitto

lrd.o: lrd.c
	$(CC) $(CFLAGS) -c lrd.c -o lrd.o
	
lib_lrdshared.a: lrd_shared.o json.o mqtt.o
	ar rcs lib_lrdshared.a lrd_shared.o json.o mqtt.o

test: test.o lib_lrdshared.a
	$(CC) $(CFLAGS) test.o lib_lrdshared.a -o test -lmosquitto
	
test.o: test.c
	$(CC) $(CFLAGS) -c test.c -o test.o

python_bind: setup.py python_bind.c lib_lrdshared.a
	CC=gcc python3 setup.py build
	cp build/lib.*/lrd*.so ./

clean:
	rm -r build
	rm lrd test lrd.*.so test.o lrd_shared.o json.o mqtt.o lrd.o lib_lrdshared.a
