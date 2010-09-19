# compile with more socket debugging
# CFLAGS=-Iinc -Wall -DDEBUG
#


all:

	make -C libami
	make -C nami

clean:
	make -C libami clean
	make -C nami clean

install:
	make -C libami install
	make -C nami install

uninstall:
	make -C libami uninstall
	make -C nami uninstall

