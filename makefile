CC=gcc
# Debian/Ubuntu etc after installing liblua5.2-dev
VS=5.1
LINC=/usr/include/lua$(VS)
LUALIB=-llua$(VS)
# release
#CFLAGS=-std=c99 -O2 -I$(LINC) -I.
#LINK=$(LUALIB) -L. -lllua -Wl,-s
# debug
CFLAGS=-std=c99 -g -I$(LINC) -I.
LINK=$(LUALIB) -L. -lllua

OBJS=llua.o llib/obj.o llib/value.o llib/pool.o
LLUA=libllua.a

all: $(LLUA) test-llua strfind tests tests-method file-size errors read-config read-config-err

clean:
	rm *.o *.a

$(LLUA): $(OBJS)
	ar rcu $(LLUA) $(OBJS) && ranlib $(LLUA)

test-llua: test-llua.o $(LLUA)
	$(CC) test-llua.o -o test-llua $(LINK)

strfind: strfind.o $(LLUA)
	$(CC) strfind.o -o strfind $(LINK)

tests: tests.o $(LLUA)
	$(CC) tests.o -o tests $(LINK)

tests-method: tests-method.o $(LLUA)
	$(CC) tests-method.o -o tests-method $(LINK)

file-size: file-size.o $(LLUA)
	$(CC) file-size.o -o file-size $(LINK)

errors: errors.o $(LLUA)
	$(CC) errors.o -o errors $(LINK)

read-config: read-config.o $(LLUA)
	$(CC) read-config.o -o read-config $(LINK)

read-config-err: read-config-err.o $(LLUA)
	$(CC) read-config-err.o -o read-config-err $(LINK)
