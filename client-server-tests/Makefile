CFLAGS = -Wall
LDLIBS =
TARGETS = test1 test2 test3 hwserver hwclient

all: $(TARGETS)

clean:
	rm -f $(TARGETS)

hwserver: hwserver.c
	${CC} ${CFLAGS} -lzmq -o hwserver hwserver.c

hwclient: hwclient.c
	${CC} ${CFLAGS} -lzmq -o hwclient hwclient.c
