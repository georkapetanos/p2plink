CC = gcc

CFLAGS = -Wall -g

LDFLAGS =  -lmosquitto -luuid -lcrypto

lrd: lrd.o lib_lrdshared.a
	$(CC) $(CFLAGS) lrd.o lib_lrdshared.a -o lrd $(LDFLAGS)

lrd.o: lrd.c
	$(CC) $(CFLAGS) -c lrd.c -o lrd.o

json.o: json.c json.h
	$(CC) $(CFLAGS) -c json.c -o json.o
	
mqtt.o: mqtt.c mqtt.h lrd_shared.h
	$(CC) $(CFLAGS) -c mqtt.c -o mqtt.o
	
crypto.o: crypto.c crypto.h
	$(CC) $(CFLAGS) -c crypto.c -o crypto.o

lib_crypto.a: crypto.o
	ar rcs lib_crypto.a crypto.o

lrd_shared.o: lrd_shared.c lrd_shared.h
	$(CC) $(CFLAGS) -c lrd_shared.c -o lrd_shared.o
	
lib_lrdshared.a: lrd_shared.o json.o mqtt.o crypto.o
	ar rcs lib_lrdshared.a lrd_shared.o json.o mqtt.o crypto.o

python_bind: setup.py python_bind.c lib_lrdshared.a lib_crypto.a
	python3 setup.py build
	cp build/lib.*/lrd*.so ./

clean:
	rm -r build
	rm lrd lrd.*.so lrd_shared.o lib_lrdshared.a json.o mqtt.o lrd.o crypto.o lib_crypto.a
