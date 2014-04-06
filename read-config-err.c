/* This is like read-config, except we do all the
    initialization in a protected call,
    and error handling can happen in one place.
    
    Just to make things more interesting, the environment has a metatable
    which turns unknown symbols into tables, so that 'A.B = 42' is
    equivalent to 'A={B=42}'. llua_gets* can do two-level lookup,
    so the key 'A.B' works as expected.
    
    There are two reference leaks in this program; we throw away
    the reference to the C function, and won't clean up the environment
    if we blow up.  Not a major problem for this once-off task.
*/
#include <stdio.h>
#include <stdlib.h>
#include "llua.h"

typedef struct {
    int alpha;
    double beta;
    char *address;
    int *ports;    
    int a_b;
} Config;

const char *meta = 
    "return setmetatable({},{ "
    " __index = function(t,k) local v = {}; rawset(t,k,v); return v end"
   "})";

int parse_config (lua_State *L) {
    llua_t *env;
    // you can of course use the llib macro obj_new(Config,NULL) here 
    Config *c = malloc(sizeof(Config));
    c->address = "127.0.0.1";
    
    env = llua_eval_or_die(L,meta,L_VAL);
    llua_set_error(env,true);   // <--- any op on 'env' will raise an error, not just return it
    llua_evalfile(L,"config.txt","",env);

     llua_gets_v(env,
        "alpha","i",&c->alpha,
        "beta","f",&c->beta,
        "address","?s",&c->address, // ? means use existing value as default!
        "ports","I",&c->ports, // big 'I' means 'array of int'
        "A.B","i", &c->a_b,
    NULL);
    
    unref(env); // don't need it any more...

    // alternatively, can create the Config pointer in main and pass as lightuserdata ('p')
    lua_pushlightuserdata(L,c);
    return 1;
}

bool handle_config(lua_State *L) {

    // doing a protected call of our function!
    Config* c = llua_callf(llua_cfunction(L,parse_config),L_NONE,L_VAL);
    if (value_is_error(c)) { // compile, run or bad field error?
        fprintf(stderr,"could not load config: %s\n",value_as_string(c));
        return false;
    }    
    
    printf("got alpha=%d beta=%f a_b=%d, address='%s'\n",
        c->alpha, c->beta,c->a_b,c->address
    );
    
    // note how you get the size of the returned array
    int *ports = c->ports;
    printf("ports ");
    for (int i = 0; i < array_len(ports); i++)
        printf("%d ",ports[i]);
    printf("\n");    
    return true;
}

int main (int argc, char **argv)
{
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    handle_config(L);
    lua_close(L);
    return 0;
}
