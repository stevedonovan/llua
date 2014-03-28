## Rationale

llua is a higher-level C API for Lua, organized around reference objects.
It was inspired by [luar](http://github.com/stevedonovan/luar) and
[LuaJava](https://github.com/jasonsantos/luajava) which
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

## References, Objects and Strings

llua references are llib objects; to free the reference use `unref`. If given
an arbitrary object, `llua_is_lua_object(obj)` will be true if a Lua reference.

 `llua_t` has the Lua state `L`, the reference
as `ref` (which is index into registry) and `type`.


`llua_new(L,idx)` will wrap a value on the stack as a reference; `llua_newtable(L)`
will make a reference to a new table, and `llua_global(L)` is the global state.
These operations don't effect the stack; `llua_push(o)` will make the reference
available on the stack.

In general, all
objects returned by llua will be allocated by llib, with the exception of strings
returned by `llua_tostring` where you get the underlying
char pointer managed by Lua.  This should be safe, since we're guaranteed
to have a reference to the string.  Lua strings can contain nuls, so to be
safe use `array_len(s)` rather than `strlen(s)` to determine length.

For accessing a large Lua string without wanting a copy, the special type
specifier 'L' will force all Lua values (including strings) to be reference
objects. Once you have the string reference, `llua_tostring` gives you
the managed pointer and `llua_len` gives you its actual length.

## Calling Lua Functions

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
`boolean` (integer value either 0 or 1), 'o' -> `object`, 'v' -> "value on stack",
and 'x' -> `C function`.

In the above example, there
are three arguments, two strings and a integer, and the result is two integers,
which are returned by reference.


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

Since this little "r" can be hard to see in code, it's given a name L_VAL.
As a special case, `llua_callf` can call methods. The pseudo-type 'm' means
"call the named method of the given object":


```C
    llua_t *out = llua_gets(G,"io.stdout");
    llua_callf(out,"ms","write","hello dolly!\n",L_NONE);

```

## Accessing Lua Tables

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
    llua_t *res = llua_eval(L,"return {a = 'one', b = 'two', c=66}",L_VAL);
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
    llua_t *res = llua_eval(L,"return {10,20,30}",L_VAL);
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

Iterating over a table is fairly straightforward using the usual API but I usually
have to look it up each time.  So llua provides a `FOR_TABLE` macro:


```C
    FOR_TABLE(G) { // all table keys matching "^s"
        void *obj = llua_callf(strfind,"vs",L_TKEY,"^s",L_VAL);
        if (obj) {
            printf("match %s\n",lua_tostring(L,L_TKEY));
            unref(obj);  
        }
    }
```

Like the explicit version, this requires stack discipline!  L_TKEY is (-2) and
L_TVAL is (-1); the 'v' type specifier expects a stack index.

If you do need to break out of this loop, use the `llua_table_break`
macro which does the necessary key-popping.

## Error Handling

Generally, all llua functions which can return an object, can also return an error; 
llib's `value_is_error` can distinguish between error strings and any other object, 
including plain strings.  We can do this because all results are llib objects and 
have dynamic type information, which provides an elegant solution to the old
"return value or error" problem with C.  (These runtime checks are pretty fast
since the type infomation is part of the hidden object header.)

Scalar values like ints and floats will also be returned this way, as llib 'boxed'
values (check with `value_is_int`, unbox with `value_as_int`, etc.)  This is
not so convenient, hence llua's use of scanf-like type specifiers with variables.

Lua has a common idiom, where normally a function will return one value, 
or `nil` plus an error string.  The `llua_callf` return type `L_ERR` makes this into
a single  value, which can be distinguished from an error as above.

However, passing a Lua API or library function a wrong parameter will result
in an error being raised.  For instance, trying to index a number, or call a `nil`
value.  So `llua_gets*` will not return an error in this case; you should query 
`llua_gettable(o)` first.  This is true if the object was a table or something with an
`__index` metamethod.  (It would be too expensive to do this check each time.)
So then you need the C equivalent of Lua's protected function calls:

```C
int l_myfun(lua_State *L) {
    const char *sval;
    llua_t *r;
    lua_pushnil(L);
    r = llua_new(L,-1);
    sval = llua_gets(r,"woop");
    printf("got '%s'\n",sval);
    return 0;
}

.....
    llua_t  *myfun = llua_cfunction(L,l_myfun);
    err_t res = llua_callf(myfun,L_NONE,L_VAL);
    if (res) {
        fprintf(stderr,"failed: %s\n",res);
    }
    --> failed: attempt to index a nil value
```

Note that `l_myfun` doesn't return any value, so `llua_callf` only returns non-nil
when there's an error.

However, if we tried to call the `nil` reference `r` above with `llua_callf`, no
error will be raised, since it does a protected call.  You can use the following form
to raise the error:

```C
    // make 'calling a nil value' a fatal error
    void *val = llua_call_or_die(r,L_NONE,L_VAL);
    ...
    // ---> failed errors.c:23: attempt to call a nil value
```

This is a macro built on `llua_assert(L,val)` which you can generally use for
converting error values into raised errors.  (Like the C macro `assert`, it adds
file:line information)

There is yet another mechanism that forces returned errors into raised errors.
If a llua reference has its `error` field set, then any operation involving it will 
raise an error. So an alternative way of doing the last operation would be:

```C
    llua_set_error(r,true);
    void *val = llua_callf(r,L_NONE,L_VAL);

```
The error message won't be so pretty, however.  (I apologize for providing
several mechanisms for achieving the same result; in the early days
of experimenting with a library interface it's useful to present alternatives.)

This is useful if you're found of exception handling as a strategy to separate
'normal' program flow from error handling.  Consider the case of reading a 
configuration file. All the straightforward business of loading and querying
happens in a protected call:

```C
typedef struct {
    int alpha;
    double beta;
    char *address;
    int *ports;    
} Config;

int parse_config (lua_State *L) {
    llua_t *env;
    Config *c = malloc(sizeof(Config));
    c->address = "127.0.0.1";
    
    env = llua_newtable(L);
    llua_set_error(env,true);   // <--- any op on 'env' will raise an error, not just return it
    llua_evalfile(L,"config.lua","",env);

     llua_gets_v(env,
        "alpha","i",&c->alpha,
        "beta","f",&c->beta,
        "address","?s",&c->address, // ? means use existing value as default!
        "ports","I",&c->ports, // big 'I' means 'array of int'
    NULL);
    
    unref(env); // don't need it any more...

    lua_pushlightuserdata(L,c);
    return 1;
}

```

And the error handling then becomes centralized:

```C
    Config* c = llua_callf(llua_cfunction(L,parse_config),L_NONE,L_VAL);
    if (value_is_error(c)) { // compile, run or bad field error?
        fprintf(stderr,"could not load config: %s\n",value_as_string(c));
        return 1;
    }    
    
    printf("got alpha=%d beta=%f address='%s'\n",
        c->alpha, c->beta,c->address
    );
    
    // note how you get the size of the returned array
    int *ports = c->ports;
    printf("ports ");
    for (int i = 0; i < array_len(ports); i++)
        printf("%d ",ports[i]);
    printf("\n");    

```

The point is that both `llua_evalfile` and `llua_gets_v` may throw errors; the first
if there's an error parsing and executing the configuration, the second
if a required field is missing or has the wrong type.  For complicated sequences
of operations, this will give you cleaner code and leave your 'happy path'
uncluttered.

Please note how easy it is for the protected function to return a pointer as
'light userdata';  alternatively we could have created the config object and
passed it as light userdata using the 'p' type specifier, and picked it up as
`lua_topointer(L,1)` in the protected code.

## Managing References

In the above example, there are two 'reference leaks'; the first comes from
throwing away the reference to `parse_config` and the second happens when
an error is raised and `env` is not dereferenced with `unref`.  This is not C++,
and we don't have any guaranteed cleanup on scope exit!

In this case (once-off reading of configuration file) it's probably no big deal,
but you do have to manage the lifetime of references in general.

One approach to scope cleanup is to use 'object pools', which is similar to how
Objective-C manages reference-counting.

```C
void my_task() {
    void *P = obj_pool();
    llua_t *result;
    
    //.... many references created
    result = ref(obj);
    
    unref(P); // all references freed, except 'result'
    return result;
}
```

Object pools can be nested (they are implemented as a stack of resizeable
reference-owning arrays) and they will do the Right Thing.  To make sure
they don't clean up _everything_, use `ref` to increase the reference count
of objects you wish to keep - in this case, the result of the function.

If you're using GCC (and compilers which remain GCC-compatible, like Clang
and later versions of ICC) there is a 'cleanup' attribute which allows us to 
leave out that explicit 'unref(P)`:

```
  {
   scoped void *P = obj_pool();
   
   // no unref needed!
   }
```

The cool thing is that we then have that favourite feature of C++ programmers:
the ability to do auto cleanup on a scope level.  This is particularly convenient if
you like the freedom to do multiple returns from a function and dislike the
bookkeeping needed to do manual cleanup.  It isn't a C standard, but its wide
implementation makes it an option for those who can choose their compilers.

(scoped_pool)

A potential problem with relying too heavily on object pools is that you may
create and destroy many temporary references, which could slow you
down in critical places.

