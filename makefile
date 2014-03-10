CC=gcc
# Debian/Ubuntu etc after installing liblua5.2-dev
VS=5.2
LINC=/usr/include/lua$(VS)
LUALIB=-llua$(VS)
CFLAGS=-std=c99 -I$(LINC) -I.
LINK=$(LUALIB) -L. -lllua

OBJS=llua.o llib/obj.o llib/value.o
LLUA=libllua.a

all: $(LLUA) test-llua strfind tests

$(LLUA): $(OBJS)
	ar rcu $(LLUA) $(OBJS) && ranlib $(LLUA)

test-llua: test-llua.o $(LLUA)
	$(CC) test-llua.o -o test-llua $(LINK)

strfind: strfind.o $(LLUA)
	$(CC) strfind.o -o strfind $(LINK)

tests: tests.o $(LLUA)
	$(CC) tests.o -o tests $(LINK)
