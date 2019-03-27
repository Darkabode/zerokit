start: main

main: config.o utils.o crc64.o ../../shared_code/config.h ../../shared_code/types.h ../../shared_code/utils.h main.c
	gcc -Wall -O2 -I/usr/local/include -L/usr/local/lib -L/usr/local/lib/mysql -lm -lz -lpthread -lmysqlclient_r -lGeoIP main.c config.o utils.o crc64.o -o ahelper

config.o: ../../shared_code/config.c ../../shared_code/config.h utils.o ../../shared_code/utils.h
	gcc -Wall -O2 -c ../../shared_code/config.c

utils.o: ../../shared_code/utils.c ../../shared_code/utils.h
	gcc -Wall -O2 -c ../../shared_code/utils.c

crc64.o: ../../shared_code/crc64.c ../../shared_code/crc64.h
	gcc -Wall -O2 -c ../../shared_code/crc64.c

clean:
	rm ahelper config.o utils.o crc64.o
