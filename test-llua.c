#include <stdio.h>
#include "llua.h"

int main (int argc, char **argv)
{
    lua_State *L = luaL_newstate();
    char *expr = "print(10+20)";

    luaL_openlibs(L);
    if (argc > 1)
        expr = argv[1];

    // lower-level llua functions are similar to corresponding Lua API calls,
    // except that llua_t objects carry the references, and we can
    // always distinguish valid results from errors by llib runtime type info
    llua_t *chunk = llua_load(L,expr, "line");
    if (value_is_error(chunk)) {
        fprintf(stderr,"compile %s\n",value_as_string(chunk));
        return 1;
    }
    err_t err = llua_callf(chunk,"","");
    if (err) {
        fprintf(stderr, "run %s\n", err);
        return 1;
    }

    const char *code = "return {10,20,30}";

    // compile the cunk
    chunk = llua_load(L,code,"tmp");

    // callf allows values to be returned directly...("" means 'no arguments' here)
    llua_t *res = llua_callf(chunk,"","r");

    // we get back a Lua table
    printf("type '%s', length %d\n",llua_typename(res),llua_len(res));

    // llua_geti, gets always returns objects.
    void *obj = llua_geti(res,1);
    printf("value was %f\n",value_as_float(obj));

    // or we can convert the table to an array of ints
    llua_push(res);
    int *arr = llua_tointarray(L,-1);
    int i;
    for (i = 0; i < array_len(arr); i++)
        printf("%d\n",arr[i]);

    // Take 2. Compile & run, and return explicitly as int array
    arr = llua_eval(L,code,"rI");
    printf("size %d type %s\n",array_len(arr),obj_type(arr)->name);

    llua_t *G = llua_global(L);
    // straightforward to call standard Lua functions ("" means 'no returns')
    res = llua_gets(G,"print");
    llua_callf(res,"ssi","one","two",42,"");

    // llua_gets will work with one extra level of lookup...
    const char *ppath = llua_gets(G,"package.path");
    llua_t* sub = llua_gets(G,"string.sub");
    res = llua_callf(sub,"sii",ppath,1,10,"r");
    printf("first part of package.path '%s'\n",res);

    // load a table...
    res = llua_eval(L,"return {a = 'one', b = 'two', c=66}","r");
    char *av, *bv;
    int cv;
    double dv = 23.5;
    llua_gets_v(res,
        "a","s",&av,
        "b","s",&bv,
        "c","i",&cv,
        "d","?f",&dv, // no error if not defined, use default!
    NULL);
    printf("got a='%s' b='%s' c=%d d=%f\n",av,bv,cv,dv);

    res = llua_newtable(L);
    // two ways to set values: first uses _dynamic_ type of
    // value
    //llua_sets(res,"one","hello");
    //llua_sets(res,"two",value_float(1.4));
    // and second works like gets_v, using explicit format.
    llua_sets_v(res,
        "one","s","hello",
        "two","f",1.4,
    NULL);

    av = llua_gets(res,"one");
    printf("got %s\n",av);

    llua_t *env = llua_newtable(L);
    err = llua_evalfile(L,"config.lua","",env);
    if (err) {
        fprintf(stderr,"could not load config.lua: %s\n",err);
    } else {
        av = llua_gets(env,"gammon");
        printf("gammon was %s\n",av);
    }
    lua_close(L);
    return 0;
}
