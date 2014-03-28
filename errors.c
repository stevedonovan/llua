/* Even unsafe operations (like indexing nil)
 can be caught by a protected call to your function.
*/
#include <stdio.h>
#include <llua.h>

typedef llua_t *llua;

int l_myfun(lua_State *L) {
    int ival;
    const char *sval;
    // nil -> integer is tolerant
    lua_pushnil(L);
    ival = lua_tointeger(L,-1);
    printf("ival %d\n",ival);

    llua r = llua_new(L,-1);
    // but trying to index an arbitrary value is a problem!
    // this makes us fail with "failed attempt to index a nil value"
    // sval = llua_gets(r,"woop");

    // make 'calling a nil value' a fatal error
    void *val = llua_call_or_die(r,L_NONE,L_VAL);
    
    // alternatively, can make the reference raise errors
    // (you will not get a nice file:line in the message however)
    //llua_set_error(r,true);
    //void *val = llua_callf(r,L_NONE,L_VAL);

    return 0;
}

int main (int argc, char **argv)
{
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    // doing a protected call of our function!
    llua myfun = llua_cfunction(L,l_myfun);
    err_t res = llua_callf(myfun,L_NONE,L_VAL);
    if (res) {
        fprintf(stderr,"failed %s\n",res);
    }
//~ --> failed errors.c:23: attempt to call a nil value
    lua_close(L);
    return 0;
}
