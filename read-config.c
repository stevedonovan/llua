#include <stdio.h>
#include "llua.h"

int main (int argc, char **argv)
{
    lua_State *L = luaL_newstate();

    // load the config file using a table as an environment
    // (it doesn't need '.lua' extension of course)
    llua_t *env = llua_newtable(L);
    const char *err = llua_evalfile(L,"config.lua","",env);
    if (err) { // a compile error?
        fprintf(stderr,"could not load config.lua: %s\n",err);
        return 1;
    }
    
    // we can now read values from the config
    int alpha;
    double beta;
    char *address = "127.0.0.1";
    int *ports;
    char *author_name, *author_email;
    
     err = llua_gets_v(env,
        "alpha","i",&alpha,
        "beta","f",&beta,
        "address","?s",&address, // ? means use existing value as default!
        "ports","I",&ports, // big 'I' means 'array of int'
        "author.name","s",&author_name,
        "author.email","s",&author_email,
    NULL);
    if (err) { // required field not specified?
        fprintf(stderr,"config field: %s\n",err);
        return 1;
    }
    
    printf("got alpha=%d beta=%f address='%s' name='%s' email='%s'\n",
        alpha, beta, address, author_name, author_email
    );
    
    // note how you get the size of the returned array
    printf("ports ");
    for (int i = 0; i < array_len(ports); i++)
        printf("%d ",ports[i]);
    printf("\n");
    
    lua_close(L);
    return 0;
}
