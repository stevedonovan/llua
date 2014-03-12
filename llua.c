// @build=lake.bat -g
#include <stdio.h>
#include <string.h>

#include "llua.h"

// Lua 5.1 compatibility
#if LUA_VERSION_NUM == 501
#define LUA_OK 0
#define lua_rawlen lua_objlen
#endif

static void llua_Dispose(llua_t *o) {
    luaL_unref(o->L,LUA_REGISTRYINDEX,o->ref);
}

/// new Lua reference to value on stack
llua_t *llua_new(lua_State *L, int idx) {
    llua_t *res = obj_new(llua_t,llua_Dispose);
    res->L = L;
    lua_pushvalue(L,idx);
    res->ref = luaL_ref(L,LUA_REGISTRYINDEX);
    res->type = lua_type(L,idx);
    return res;
}

/// is this a Lua reference?
bool llua_is_lua_object(llua_t *o) {
    return obj_is_instance(o,"llua_t");
}

/// get the global state as a reference.
llua_t* llua_global(lua_State *L) {
    llua_t *res;
    lua_getglobal(L,"_G");
    res = llua_new(L,-1);
    lua_pop(L,1);
    return res;
}

llua_t *llua_getmetatable(llua_t *o) {
    lua_State *L = llua_push(o);
    if (! lua_getmetatable(L,-1)) {
        return NULL;
    } else {
        llua_t *res = llua_new(L,-1);
        lua_pop(L,1);
        return res;
    }
}

void llua_setmetatable(llua_t *o, llua_t *mt) {
    lua_State *L = llua_push(o);
    if (mt)
        llua_push(mt);
    else
        lua_pushnil(L);
    lua_setmetatable(L,-2);
    lua_pop(L,1);
}

// Lua strings may have embedded nuls, so don't
// depend on lua_tostring!
static char *string_copy(lua_State *L, int idx) {
    size_t sz;
    const char *s = lua_tolstring(L,idx,&sz);
    char *res = str_new_size(sz);
    memcpy(res,s,sz);
    return res;
}

/// value on stack as a llib object, or Lua reference
void *llua_to_obj(lua_State *L, int idx) {
    switch(lua_type(L,idx)) {
    case LUA_TNIL: return NULL;
    case LUA_TNUMBER: return value_float(lua_tonumber(L,idx));
    case LUA_TBOOLEAN: return value_bool(lua_toboolean(L,idx));
    case LUA_TSTRING: return string_copy(L,idx);
    default:
        return llua_new(L,idx);
     //LUA_TTABLE, LUA_TFUNCTION, LUA_TUSERDATA, LUA_TTHREAD, and LUA_TLIGHTUSERDATA.
    }
}

/// convenient way to call `llua_to_obj` which pops the stack.
void *llua_to_obj_pop(lua_State *L, int idx) {
    void *res = llua_to_obj(L,idx);
    lua_pop(L,1);
    return res;
}

/// type name of the reference.
const char *llua_typename(llua_t *o) {
    return lua_typename(o->L,o->type);
}

/// a reference to a new Lua table.
llua_t *llua_newtable(lua_State *L) {
    llua_t *ref;
    lua_newtable(L);
    ref = llua_new(L,-1);
    lua_pop(L,1);
    return ref;
}

static err_t l_error(lua_State *L) {
    const char *errstr = value_error(lua_tostring(L,-1));
    lua_pop(L,1);
    return errstr;
}

/// load a code string and return the compiled chunk as a reference.
llua_t *llua_load(lua_State *L, const char *code, const char *name) {
    int res = luaL_loadbuffer(L,code,strlen(code),name);
    if (res != LUA_OK) {
        return (llua_t*)l_error(L);
   }
   return llua_to_obj_pop(L,-1);
}

/// load a file and return the compiled chunk as a reference.
llua_t *llua_loadfile(lua_State *L, const char *filename) {
    int res = luaL_loadfile(L,filename);
    if (res != LUA_OK) {
        return (llua_t*)l_error(L);
   }
   return llua_to_obj_pop(L,-1);
}

/// push the reference on the stack.
lua_State *llua_push(llua_t *o) {
    lua_rawgeti(o->L,LUA_REGISTRYINDEX,o->ref);
    return o->L;
}

/// length of Lua reference, if a table or userdata.
// Note that we are specifically not using `lua_rawlen` here!
int llua_len(llua_t *o) {
    llua_push(o);
#if LUA_VERSION_NUM == 501
    int n = lua_objlen(o->L,-1);
#else
    int n = luaL_len(o->L,-1);
#endif
    lua_pop(o->L,1);
    return n;
}

/// Lua table as an array of doubles.
double *llua_tonumarray(lua_State* L, int idx) {
    int i,n = lua_rawlen(L,idx);
    double *res = array_new(double,n);
    for (i = 0; i < n; i++) {
        lua_rawgeti(L,idx,i+1);
        res[i] = lua_tonumber(L,-1);
        lua_pop(L,1);
    }
    return res;
}

/// Lua table as an array of ints.
int *llua_tointarray(lua_State* L, int idx) {
    int i,n = lua_rawlen(L,idx);
    int *res = array_new(int,n);
    for (i = 0; i < n; i++) {
        lua_rawgeti(L,idx,i+1);
        res[i] = lua_tointeger(L,-1);
        lua_pop(L,1);
    }
    return res;
}

/// Lua table as an array of strings.
char** llua_tostrarray(lua_State* L, int idx) {
    int i,n = lua_rawlen(L,idx);
    char** res = array_new_ref(char*,n);
    for (i = 0; i < n; i++) {
        lua_rawgeti(L,idx,i+1);
        res[i] = string_copy(L,-1);
        lua_pop(L,1);
    }
    return res;
}

static int is_indexable(lua_State *L, int idx) {
    return lua_istable(L,-1) || lua_isuserdata(L,-1);
}

err_t llua_convert(lua_State *L, char kind, void *P, int idx) {
    err_t err = NULL;
    switch(kind) {
    case 'i':
        if (! lua_isnumber(L,idx))
            err = "not a number!";
        else
            *((int*)P) =  lua_tointeger(L,idx);
        break;
    case 'f':
        if (! lua_isnumber(L,idx))
            err = "not a number!";
        else
            *((double*)P) = lua_tonumber(L,idx);
        break;
    case 's':
        if (! lua_isstring(L,idx))
            err = "not a string!";
        else
            *((char**)P) = string_copy(L,idx);
        break;
    case 'o':
        *((llua_t**)P) = llua_to_obj(L,idx);
    case 'F':
        if (! is_indexable(L,idx))
            err = "not indexable!";
        else
            *((double**)P) = llua_tonumarray(L,idx);
        break;
    case 'I':
        if (! is_indexable(L,idx))
            err = "not indexable!";
        else
            *((int**)P) = llua_tointarray(L,idx);
        break;
    case 'S':
        if (! is_indexable(L,idx))
            err = "not indexable!";
        else
            *((char***)P) = llua_tostrarray(L,idx);
        break;
    default:
      break;
    }
    return err ? value_error(err) : NULL;
}

static err_t push_value(lua_State *L, char kind, void *data) {
    switch(kind) {
    case 's':
        lua_pushstring(L, (const char*)data);
        break;
    case 'o':
        llua_push((llua_t*)data);
        break;
    case 'x': // usually possible to do this cast
        lua_pushcfunction(L,(lua_CFunction)data);
        break;
    default:
        return value_error("unknown type");
    }
    return NULL;
}

/// call the reference, passing a number of arguments.
// May optionally capture multiple return values.
void *llua_callf(llua_t *o, const char *fmt,...) {
    lua_State *L = o->L;
    int nargs = 0, nres = LUA_MULTRET, nerr;
    err_t res = NULL;
    va_list ap;
    va_start(ap,fmt);
    llua_push(o); // push the function or object
    if (*fmt == 'm') { // method call!
        const char *name = va_arg(ap,char*);
        lua_getfield(L,-1,name);
        // method at top, then self
        lua_insert(L,-2);
        ++fmt;
        ++nargs;
    }
    // and push the arguments...
    while (*fmt) {
        switch(*fmt) {
        case 'i':
            lua_pushinteger(L, va_arg(ap,int));
            break;
        case 'b':
            lua_pushboolean(L, va_arg(ap,int));
            break;
        case 'f':
            lua_pushnumber(L, va_arg(ap,double));
            break;
        default:
            res = push_value(L,*fmt, va_arg(ap,void*));
            if (res)
                return (void*)res;
            break;
        }
        ++fmt;
        ++nargs;
    }
    fmt = va_arg(ap,char*);
    if (fmt) {
        nres = strlen(fmt);
        if (*fmt == 'r' && *(fmt+1)) // single return with explicit type
            --nres;
    }
    nerr = lua_pcall(L,nargs,nres,0);
    if (nerr != LUA_OK) {
        res = l_error(L);
    }
    if (nres == LUA_MULTRET || res != NULL) { // leave results on stack, or error!
        return (void*)res;
    } else
    if (*fmt == 'r') { // return one value as object...
        if (*(fmt+1)) { // force the type!
            void *value;
            res = llua_convert(L,*(fmt+1),&value,-1);
            lua_pop(L,1);
            if (res) // failed...
                return (void*)res;
            else
                return value;
        } else {
            return llua_to_obj_pop(L,-1);
        }
    } else
    if (nres != LUA_MULTRET) {
        int idx = -nres;
        while (*fmt) {
            res = llua_convert(L,*fmt,va_arg(ap,void*),idx);
            if (res) // conversion error!
                break;
            ++fmt;
            --idx;
        }
        lua_pop(L,nres);
    }
    va_end(ap);
    return (void*)res;
}

/// pop some values off the Lua stack.
err_t llua_pop_vars(lua_State *L, const char *fmt,...) {
    err_t res = NULL;
    va_list ap;
    va_start(ap,fmt);
    while (*fmt) {
        res = llua_convert(L,*fmt,va_arg(ap,void*),-1);
        if (res) // conversion error!
            break;
        ++fmt;
        lua_pop(L,1);
    }
    va_end(ap);
    return res;
}

/// this returns the _original_ raw C string.
const char *llua_tostring(llua_t *o) {
    const char *res;
    llua_push(o);
    res = lua_tostring(o->L,-1);
    lua_pop(o->L,1);
    return res;
}

/// the Lua reference as a number.
lua_Number llua_tonumber(llua_t *o) {
    lua_Number res;
    llua_push(o);
    res = lua_tonumber(o->L,-1);
    lua_pop(o->L,1);
    return res;
}

/// can we index this object?
static bool indexable(llua_t *o, const char *metamethod) {
    if (o->type == LUA_TTABLE) { // always cool
        return true;
    } else {
        lua_State *L = llua_push(o);
        if (! luaL_getmetafield(L,-1,metamethod)) { // no metatable!
            lua_pop(L,1);
            return false;
        }
        lua_pop(L,2); // metamethod and table
        return true;
    }
}

/// can we get a field of this object?
bool llua_gettable(llua_t *o) {
    return indexable(o,"__index");
}

/// can we set a field of this object?
bool llua_settable(llua_t *o) {
    return indexable(o,"__newindex");
}

static char *splitdot(char *key) {
    char *p = strchr(key,'.');
    if (p) { // sub.key
        *p = '\0';
        return p+1;
    } else
        return NULL;
}

#define MAX_KEY 256

// assume the table is initially on top of the stack...
// leaves final value on top
static void safe_gets(lua_State *L, const char *key) {
    char ckey[MAX_KEY], *subkey;
    strcpy(ckey,key);
    subkey = splitdot(ckey);
    lua_getfield(L,-1,ckey);
    if (subkey) {
        if (! lua_isnil(L,-1)) {
            lua_getfield(L,-1,subkey);
            lua_remove(L,-2);
        }
    }
}

/// index the reference with a string key, returning an object.
void *llua_gets(llua_t *o, const char *key) {
    lua_State *L = llua_push(o);
    safe_gets(L,key);
    lua_remove(L,-2);
    return llua_to_obj_pop(L,-1);
}

/// index the reference with multiple string keys and type-specifiers.
err_t llua_gets_v(llua_t *o, const char *key,...) {
    lua_State *L = llua_push(o);
    const char *fmt;
    void *P;
    bool convert;
    err_t err = NULL;
    va_list ap;
    va_start(ap,key);
    while (key) { // key followed by type specifier and pointer-to-data
        fmt = va_arg(ap,const char*);
        P = va_arg(ap,void*);
        convert = true;
        safe_gets(L,key);
        if (*fmt == '?') {
            ++fmt;
            if (lua_isnil(L,-1)) // fine! leave value alone!
                convert = false;
        }
        if (convert)
            err = llua_convert(L,*fmt,P,-1);
        lua_pop(L,1);
        if (err) {
            break;
        }
        key = va_arg(ap,const char*);
    }
    va_end(ap);
    lua_pop(L,1); // the reference
    return err;
}

/// index the reference with an integer key.
void *llua_geti(llua_t *o, int key) {
    lua_State *L = llua_push(o);
    lua_pushinteger(L,key);
    lua_gettable(L,-2);
    lua_remove(L,-2); // the reference
    return llua_to_obj_pop(L,-1);
}

/// raw indexing with an integer key.
void *llua_rawgeti(llua_t* o, int key) {
    lua_State *L = llua_push(o);
    lua_rawgeti(L,-1,key);
    lua_remove(L,-2); // the reference
    return llua_to_obj_pop(L,-1);
}

/// push an llib object.
// equivalent to `llua_push` if it's a llua ref, otherwise
// uses llib type. If there's no type it assumes a plain
// string literal.
void llua_push_object(lua_State *L, void *value) {
    if (llua_is_lua_object((llua_t*)value)) {
        llua_push((llua_t*)value);
    }  else
    if (value_is_float(value)) {
        lua_pushnumber(L,value_as_float(value));
    } else
    if (value_is_string(value)) {
        lua_pushstring(L,(const char*)value);
    } else
    if (value_is_int(value)) {
        lua_pushnumber(L,value_as_int(value));
    } else
    if (value_is_bool(value)) {
        lua_pushboolean(L,value_as_bool(value));
    }  else { // _probably_ a string ;)
        lua_pushstring(L,(const char*)value);
    }
}

/// set value using integer key.
// uses `llua_push_object`
void llua_seti(llua_t *o, int key, void *value) {
    lua_State *L = llua_push(o);
    lua_pushinteger(L,key);
    llua_push_object(L,value);
    lua_settable(L,-3);
}
/// set value using integer key.
// uses `llua_push_object`
void llua_sets(llua_t *o, const char *key, void *value) {
    lua_State *L = llua_push(o);
    llua_push_object(L,value);
    lua_setfield(o->L,-2,key);
}

// there's some code duplication here with llua_callf, but I'm not
// 100% sure how far to push <stdarg.h>!

/// set multiple keys and values on the reference.
// uses type specifiers like `llua_callf`
err_t llua_sets_v(llua_t *o, const char *key,...) {
    lua_State *L = llua_push(o);
    const char *fmt;
    err_t err = NULL;
    va_list ap;
    va_start(ap,key);
    while (key) { // key followed by type specifier and pointer-to-data
        fmt = va_arg(ap,const char*);
        switch(*fmt) {
        case 'i':
            lua_pushinteger(L, va_arg(ap,int));
            break;
        case 'b':
            lua_pushboolean(L, va_arg(ap,int));
            break;
        case 'f':
            lua_pushnumber(L, va_arg(ap,double));
            break;
        default:
            err = push_value(L,*fmt,va_arg(ap,void*));
            break;
        }
        lua_setfield(o->L,-2,key);
        key = va_arg(ap,const char*);
    }
    va_end(ap);
    lua_pop(L,1); // the reference
    return err;
}

/// load and evaluate an expression.
// `fret` is a type specifier for the result, like `llua_callf`.
void *llua_eval(lua_State *L, const char *expr, const char *fret) {
    llua_t *chunk = llua_load(L,expr,"tmp");
    if (value_is_error(chunk)) // compile failed...
        return chunk;
    void *res = llua_callf(chunk,"",fret);
    obj_unref(chunk); // free the chunk reference...
    return res;
}

/// load and evaluate a file in an environment
// `env` may be NULL.
// `fret` is a type specifier for the result, like `llua_callf`.
void *llua_evalfile(lua_State *L, const char *file, const char *fret, llua_t *env) {
    llua_t *chunk = llua_loadfile(L,file);
    if (value_is_error(chunk)) // compile failed...
        return chunk;
    if (env) {
        llua_push(chunk);
        llua_push(env);
#if LUA_VERSION_NUM == 501
        lua_setfenv(L,-2);
#else
        // _ENV is first upvalue of main chunks
        lua_setupvalue(L,-2,1);
#endif
        lua_pop(L,1);
    }
    void *res = llua_callf(chunk,"",fret);
    obj_unref(chunk);
    return res;
}

