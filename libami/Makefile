# compile with more socket debugging
#


DB="db4.6"

CFLAGS=-I/usr/include/db4.6 -I. -I../inc -Wall -DDEBUG

CFLAGS=-I/usr/include/db4.6 -I. -I../inc -Wall

LDFLAGS=-ldb-4.6


#LIB_OBJECTS=nami.o sock.o dbg.o manager_func.o manager_status.o md5.o
LIB_OBJECTS=nami.o sock.o dbg.o manager_func.o manager_status.o 


all: libami.so


libami.so: ${LIB_OBJECTS}
	gcc ${CFLAGS} -Iinc -fPIC -g -c -Wall nami.c
	gcc ${CFLAGS} -Iinc -fPIC -g -c -Wall manager_func.c
	gcc ${CFLAGS} -Iinc -fPIC -g -c -Wall manager_status.c
	gcc ${CFLAGS} -Iinc -fPIC -g -c -Wall sock.c
	gcc ${CFLAGS} -Iinc -fPIC -g -c -Wall dbg.c
#	gcc ${CFLAGS} -Iinc -fPIC -g -c -Wall md5.c

	gcc ${CFLAGS} -Iinc -shared -Wl,-soname,libami.so -o libami.so ${LIB_OBJECTS} ${LDFLAGS}

clean:
	rm  -f *.o *.so 


install: all
	cp libami.so /usr/lib
	ldconfig

uninstall:
	rm -f /usr/lib/libami.so


