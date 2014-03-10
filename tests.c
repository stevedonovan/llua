#include <stdio.h>
#include <ctype.h>
#include <llua.h>

int l_test(lua_State *L) {
    const char *s = lua_tostring(L,1);
    char *cs = str_new(s);
    *cs = toupper(*cs);
    lua_pushstring(L,cs);
    unref(cs);
    return 1;
}

int main (int argc, char **argv)
{
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    llua_t *G = llua_global(L);
    llua_t *strfind = llua_gets(G,"string.find");
    
    ///////// Gettable and Settable ////////
    // functions are _usually_ a no-no
    printf("get strfind %d\n",llua_gettable(strfind));
    
    lua_pushstring(L,"hello dolly");
    llua_t *str = llua_new(L,-1);
    
    // strings are fine, although not very useful
    printf("get str %d\n",llua_gettable(str));
    
    // but they aren't settable...
    printf("set str %d\n",llua_settable(str));
    
    // this userdata is ok because it has a metatable with __index defined
    llua_t *out = llua_gets(G,"io.stdout");
    printf("get out %d\n",llua_gettable(out));
    
    llua_t *gsub = llua_gets(G,"string.gsub");
    
    const char *s = llua_callf(gsub,"ssx",
        "$home is where the $heart is","%$(%a+)",l_test,"r");
        
    printf("and we got '%s'\n",s);

    lua_close(L);
}
