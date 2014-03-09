llua is a higher-level C API for Lua, organized around reference objects.
It was inspired by [luar](http://github.com/stevedonovan/luar) and
[LuaJava](https://github.com/jasonsantos/luajava‎) which
provide similar operations when using Lua with Go and Java. The idea
is to encapsulate a Lua reference as an object which supports general
operations such as indexing (setting and getting), size, pushing on
the Lua stack and calling.

For instance, a common use of Lua is to load configuration files:

```lua
-- config.lua
alpha = 1
beta = 2
gammon = 'pork'

```

Here is how this file can be loaded into a custom environment:

```C
    llua_t *env = llua_newtable(L);
    err_t err = llua_evalfile(L,"config.lua","",env);
    if (err) {
        fprintf(stderr,"could not load config.lua: %s\n",err);
    } else {
        av = llua_gets(env,"gammon");
        printf("gammon was %s\n",av);
    }
    //--> gammon was pork

```

This example also works with both Lua 5.1 and 5.2, by hiding the
difference in how 'environments' work with the API.

llua conceals tedious and error-prone Lua stack operations when calling
Lua functions from C:

```C
    llua_t *G = llua_global(L);
    llua_t *strfind = llua_gets(G,"string.find");
    int i1,i2;

    llua_callf(strfind,"ssi","hello dolly","doll",1,"ii",&i1,&i2);

    printf("i1 %d i2 %d\n",i1,i2);
    //--> i1 7 i2 10
```

`llua_callf` takes a callable reference (a function or something which
has a `__call` metamethod), passes arguments specified by a type string,
and can return a number of values. The 'type string' is akin to `printf`
style formats: 'i' -> `int`, `f` -> `double`, `s` -> `string`, `b` ->
`boolean` (integer value either 0 or 1).  In the above example, there
are three arguments, two strings and a integer, and the result is two integers,
which are returned by reference.

