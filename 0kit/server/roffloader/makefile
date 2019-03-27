start: main

main: config.o utils.o timing.o ../../shared_code/config.h ../../shared_code/types.h ../../shared_code/utils.h main.c
	gcc -Wall -O2 -static -L/usr/lib -L/usr/local/lib -L/usr/local/lib/mysql -L/usr/lib/mysql -L/usr/mysql/lib/mysql -I/usr/local/include main.c config.o utils.o timing.o -lmysqlclient -lpthread -lhiredis -lz -lm -o roffloader

config.o: ../../shared_code/config.c ../../shared_code/config.h utils.o ../../shared_code/utils.h
	gcc -Wall -O2 -c ../../shared_code/config.c

utils.o: ../../shared_code/utils.c ../../shared_code/utils.h
	gcc -Wall -O2 -c ../../shared_code/utils.c

timing.o: ../../shared_code/timing.c ../../shared_code/timing.h
	gcc -Wall -O2 -c ../../shared_code/timing.c

clean:
	rm config.o utils.o roffloader
