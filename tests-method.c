#include <stdio.h>
#include "llua.h"

int main (int argc, char **argv)
{
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    typedef llua_t *llua;

    llua G = llua_global(L);
    llua out = llua_gets(G,"io.stdout");

    err_t res = llua_callf(out,"ms","write","hello dolly!\n","");
    if (res) {
        fprintf(stderr,"error %s\n",res);
    }

    const char *file = "tests-method.exe";
    if (argv[1])
        file = argv[1];
    llua open = llua_gets(G,"io.open");
    llua in = llua_callf(open,"ss",file,"rb","r");
    char* text = llua_callf(in,"ms","read","*a","r");
    llua_callf(in,"m","close","");
    printf("size was %d\n",array_len(text));
    unref(text);

    lua_close(L);
}
