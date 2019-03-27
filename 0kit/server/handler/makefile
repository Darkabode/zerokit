start: main

main: hiredis.o net.o sds.o fcgiunix.o fcgiw.o geoip.o config.o utils.o base64.o salsa20.o crc64.o logger.o db.o mempool.o ../../shared_code/config.h ../../shared_code/base64.h ../../shared_code/salsa20.h ../../shared_code/types.h ../../shared_code/utils.h db.h geoip.h mempool.h logger.h main.c
	gcc -Wall -O2 -static -L/usr/lib -L/usr/local/lib -L/usr/local/lib/mysql -L/usr/lib/mysql -L/usr/mysql/lib/mysql -I/usr/local/include main.c hiredis.o net.o sds.o fcgiunix.o fcgiw.o db.o geoip.o config.o utils.o base64.o salsa20.o crc64.o logger.o mempool.o -lmysqlclient -lpthread -lGeoIP -lz -lm -o handler

hiredis.o: ../../shared_code/hiredis/hiredis.c ../../shared_code/hiredis/hiredis.h
	gcc -Wall -O2 -c ../../shared_code/hiredis/hiredis.c

net.o: ../../shared_code/hiredis/net.c ../../shared_code/hiredis/net.h
	gcc -Wall -O2 -c ../../shared_code/hiredis/net.c

sds.o: ../../shared_code/hiredis/sds.c ../../shared_code/hiredis/sds.h
	gcc -Wall -O2 -c ../../shared_code/hiredis/sds.c

fcgiunix.o: fcgiunix.c fcgimisc.h fastcgi.h fcgios.h fcgi_config.h
	gcc -Wall -O2 -c fcgiunix.c

fcgiw.o: fcgiw.c fcgimisc.h fastcgi.h fcgios.h fcgi_config.h
	gcc -Wall -O2 -c fcgiw.c

geoip.o: geoip.c geoip.h
	gcc -Wall -O2 -I/usr/local/include -c geoip.c

config.o: ../../shared_code/config.c ../../shared_code/config.h utils.o ../../shared_code/utils.h
	gcc -Wall -O2 -c ../../shared_code/config.c

utils.o: ../../shared_code/utils.c ../../shared_code/utils.h
	gcc -Wall -O2 -c ../../shared_code/utils.c

base64.o: ../../shared_code/base64.c ../../shared_code/base64.h
	gcc -Wall -O2 -c ../../shared_code/base64.c

salsa20.o: ../../shared_code/salsa20.c ../../shared_code/salsa20.h
	gcc -Wall -O2 -c ../../shared_code/salsa20.c

crc64.o: ../../shared_code/crc64.c ../../shared_code/crc64.h
	gcc -Wall -O2 -c ../../shared_code/crc64.c

db.o: db.h logger.h logger.o db.c
	gcc -Wall -O2 -I/usr/local/include -c db.c

logger.o: logger.c logger.h
	gcc -Wall -O2 -c logger.c

mempool.o: mempool.c mempool.h
	gcc -Wall -O2 -c mempool.c

clean:
	rm fcgiunix.o fcgiw.o geoip.o config.o utils.o base64.o salsa20.o crc64.o logger.o db.o mempool.o handler
