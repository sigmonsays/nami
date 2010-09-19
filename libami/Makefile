# compile with more socket debugging
#


# 4.4, 4.6 and 4.8 have been known to work
dbCFLAGS=-I/usr/include/db4.8
dbLDFLAGS=-ldb-4.8


CFLAGS=${dbCFLAGS} -I. -I../inc -Wall -DDEBUG

CFLAGS=${dbCFLAGS} -I. -I../inc -Wall

LDFLAGS=${dbLDFLAGS}

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


