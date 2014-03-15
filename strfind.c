#include <stdio.h>
#include <llua.h>

int main (int argc, char **argv)
{
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    llua_t *G = llua_global(L);
    llua_t *strfind = llua_gets(G,"string.find");
    printf("start %d\n",lua_gettop(L));

    int i1,i2;
    llua_callf(strfind,"ssi","hello dolly","doll",1,"ii",&i1,&i2);

    printf("i1 %d i2 %d\n",i1,i2);

    // global keys beginning with 's'.
    // note 'v' for 'push value at index'
    // <any> -> <int> conversions are tolerant, so you get
    // zero if not an integer, e.g. when nil.  This fits
    // nicely with what C regards as true/false.
    FOR_TABLE(G) {
        int i;
        llua_callf(strfind,"vs",L_TKEY,"^s","i",&i);
        if (i)
            printf("match %s\n",lua_tostring(L,L_TKEY));
    }

    lua_close(L);
}
