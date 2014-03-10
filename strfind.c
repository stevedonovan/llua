#include <stdio.h>
#include "llua.h"

int main (int argc, char **argv)
{
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    llua_t *G = llua_global(L);
    llua_t *strfind = llua_gets(G,"string.find");

    int i1,i2;
    llua_callf(strfind,"ssi","hello dolly","doll",1,"ii",&i1,&i2);

    printf("i1 %d i2 %d\n",i1,i2);

    if (llua_gettable(strfind)) {
        const char *s = llua_gets(strfind,"hello");
        fprintf(stderr,"res %s\n",s);
    } else {
        fprintf(stderr,"cannot index this object!\n");
    }

    lua_close(L);
}
