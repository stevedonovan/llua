#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
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
    assert(! llua_gettable(strfind));
    
    lua_pushstring(L,"hello dolly");
    llua_t *str = llua_new(L,-1);
    
    // strings are fine, although not very useful
    assert(llua_gettable(str));
    
    // but they aren't settable...
    assert(! llua_settable(str));
    
    // this userdata is ok because it has a metatable with __index defined
    llua_t *out = llua_gets(G,"io.stdout");
    assert(llua_gettable(out));    
    
    //////// passing C function to a Lua function ////////////
    llua_t *gsub = llua_gets(G,"string.gsub");
    
    const char *s = llua_callf(gsub,"ssx",
        "$home is where the $heart is","%$(%a+)",l_test,"r");
        
     assert(strcmp(s,"Home is where the Heart is")==0);   
    
    //////// strings with embedded nuls    
    const char *B = "one\0two\0three";
    const char *C;
    int n = 13;
    lua_pushlstring(L,B,n);    
    llua_pop_vars(L,"s",&C,NULL);
    assert(array_len(C) == n);   
    unref(C); // we own the string - clean it up

    lua_close(L);
}
