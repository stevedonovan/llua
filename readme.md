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
`boolean` (integer value either 0 or 1), 'o' -> `object`.  In the above example, there
are three arguments, two strings and a integer, and the result is two integers,
which are returned by reference.

llua references are llib objects; to free the reference use `unref`. In general, all
objects returned by llua will be allocated by llib, with the exception of strings
returned by _reference_ by `llua_callf`,`llua_gets_v`, and explictly by 
`llua_tostring` where you get the underlying char pointer managed by Lua.

`llua_callf` can return a single value, by using the special type "r". 
Because llib objects have run-time info, the return value can always be distinguished
from an error, which is llib's solution to C's single-value-return problem.

```C
    const char *res = llua_callf(my_tostring,"o",obj,"r");
    if (value_is_error(res)) {
        fprintf(stderr,"my_tostring failed: %s\n",res);
    } else {
        // ok!
    }
```

We've already seen `llua_gets` for indexing tables and userdata; it will return
an object as above; numbers will be returned as llib boxed values, which is not so 
very convenient.  `llua_gets_v` allows multiple key lookups with type specified as
with `llua_callf`.

```C       
    char *av, *bv;
    int cv;
    double dv = 23.5;
    err_t *err;
    // load a table...
    llua_t *res = llua_eval(L,"return {a = 'one', b = 'two', c=66}","r");
    err = llua_gets_v(res,
        "a","s",&av,
        "b","s",&bv,
        "c","i",&cv,
        "d","?f",&dv, // no error if not defined, use default!
    NULL);
    printf("got a='%s' b='%s' c=%d d=%f\n",av,bv,cv,dv);
    //--> got a='one' b='two' c=66 d=23.500000
```

Note the special '?' which allows the existing value to pass through unchanged
if the key does not exist.  If there was no default, then the returned error will
be non-NULL. 

There is also `llua_geti` and `llua_rawgeti`, which also return objects. 

```C
    llua_t *res = llua_eval(L,"return {10,20,30}","r");
    void *obj = llua_geti(res,1);  // always returns an object
    if (value_is_float(obj))   // paranoid!
        printf("value was %f\n",value_as_float(obj)); // unbox
        
     // or we can convert the table to an array of ints
    llua_push(res);
    int *arr = llua_tointarray(L,-1);
    int i;
    for (i = 0; i < array_len(arr); i++)
        printf("%d\n",arr[i]);       
```

There are three table-to-array functions, `llua_tointarray`, `llua_tonumarray` and
`llua_tostrarray` which return llib arrays. Again, using llib means that these
arrays know how long they are!

A particularly intense one-liner implicitly uses this table-to-int-array conversion: 
you may force the return type with a type specifier after 'r'.

```
 int* arr = llua_eval(L,"return {10,20,30}","rI");
 ```

Trying to index a non-indexable object will cause a Lua panic, so `llua_gettable` and
`llua_settable` are defined so you can program defensively.
