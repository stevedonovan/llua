CC=gcc
# Debian/Ubuntu etc
LINC=/usr/include/lua5.1
LUALIB=-llua5.1
CFLAGS=-std=c99 -I$(LINC) -I.
LINK=$(LUALIB) -L. -lllua

OBJS=llua.o llib/obj.o llib/value.o
LLUA=liblua.a

all: llua test-llua strfind

llua: $(OBJS)
	ar rcu $(LLUA) $(OBJS) && ranlib $(LLUA)

test-llua: test-llua.o $(LLUA)
	$(CC) test-llua.o -o test-llua $(LINK)

strfind: strfind.o $(LLUA)
	$(CC) strfind.o -o strfind $(LINK)
