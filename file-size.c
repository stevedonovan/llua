#include <stdio.h>
#include <llua.h>

typedef llua_t *llua;

int main (int argc, char **argv)
{
    const char *file = argv[1] ? argv[1] : argv[0];
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    void *P = obj_pool();
    
    llua_verbose(stderr); // tell us of unrefs...

    llua _G = llua_global(L);    
    #define G(name) llua_gets(_G,name)
   
    // f = io.open(file,'rb');  text = in:read('*a'); f:close()
    llua in = llua_callf(G("io.open"),"ss",file,"rb","r");
    llua text = llua_callf(in,"ms","read","*a","rL"); //<-- note 'L'
    llua_callf(in,"m","close","");
    
    // text here is a reference to a Lua string,
    // can use llua_tostring() to get pointer.
    // Note this works with an arbitrary binary file!
    printf("size of %s was %d\n",file,llua_len(text));

    unref(P); // all refs are freed...    
//~ free L 0x880e008 ref 1 type table
//~ free L 0x880e008 ref 2 type function
//~ free L 0x880e008 ref 3 type userdata
//~ free L 0x880e008 ref 4 type string
    
    lua_close(L);
}
