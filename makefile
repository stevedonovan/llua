LINC=/me/dev/luabuild/lua-5.2.2
LUALIB=/me/dev/luabuild/bin/lua52.dll
CC=gcc
#c99.program{'llua',src='test-llua llua llib/obj llib/value',odir=true,incdir={LINC,"."},libflags=LIB}

CFLAGS=-std=c99 -I$(LINC) -I.

OBJS=llua.o llib/obj.o llib/value.o

llua: $(OBJS)
	ar rcu libllua.a $(OBJS) && ranlib libllua.a

test-llua: test-llua.o
	$(CC) test-llua.o -o test-llua $(LUALIB) -L. -lllua

