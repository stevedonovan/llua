/***
llua provides a higher-level way to integrate Lua into C projects,
by defining reference objects and providing operations like calling,
accessing, etc on them.  Most explicit manipulation of the Lua
stack becomes unnecessary.

@license BSD
@copyright Steve Donovan,2014
*/

#include <stdio.h>
#include <string.h>

#include "llua.h"

// Lua 5.1 compatibility
#if LUA_VERSION_NUM == 501
#define LUA_OK 0
#define lua_rawlen lua_objlen
#endif

static FILE *s_verbose = false;

/// raise an error when the argument is an error.
// @function llua_assert

void *_llua_assert(lua_State *L, const char *file, int line, void *val) {
    if (! value_is_error(val))
        return val;
    else
        return luaL_error(L,"%s:%d: %s",file,line,value_as_string(val));
}

/// report whenever a Lua reference is freed
void llua_verbose(FILE *f) {
    s_verbose = f;
}

void llua_set_error(llua_t *o, bool yesno) {
    o->error = yesno;
}

const char *llua_error(llua_t *o, const char *msg) {
    if (! (o && o->error && msg && value_is_error(msg)))
        return msg;
    return luaL_error(o->L,msg);
}

/// is this a Lua reference?
// @within Properties
bool llua_is_lua_object(llua_t *o) {
    return obj_is_instance(o,"llua_t");
}

static void llua_Dispose(llua_t *o) {
    if (s_verbose) {
        fprintf(s_verbose,"free L %p ref %d type %s\n",o->L,o->ref,llua_typename(o));
    }
    luaL_unref(o->L,LUA_REGISTRYINDEX,o->ref);
}

/// new Lua reference to value on stack.
// @within Creating
llua_t *llua_new(lua_State *L, int idx) {
    llua_t *res = obj_new(llua_t,llua_Dispose);
    res->L = L;
    lua_pushvalue(L,idx);
    res->ref = luaL_ref(L,LUA_REGISTRYINDEX);
    res->type = lua_type(L,idx);
    res->error = false;
    return res;
}

/// get the global state as a reference.
// @within Creating
llua_t* llua_global(lua_State *L) {
    llua_t *res;
    lua_getglobal(L,"_G");
    res = llua_new(L,-1);
    lua_pop(L,1);
    return res;
}

/// get the metatable of `o`
// @within Properties
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

/// set the metatable of `o`
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

/// value on stack as a llib object, or Lua reference.
// Result can be NULL, a string, a llib boxed value,
// or a `llua_t` reference.
// @within Converting
void *llua_to_obj(lua_State *L, int idx) {
    switch(lua_type(L,idx)) {
    case LUA_TNIL: return NULL;
    case LUA_TNUMBER: return value_float(lua_tonumber(L,idx));
    case LUA_TBOOLEAN: return value_bool(lua_toboolean(L,idx));
    case LUA_TSTRING: return string_copy(L,idx);
    case LUA_TLIGHTUSERDATA: return lua_topointer(L,idx);
    default:
        return llua_new(L,idx);
     //LUA_TTABLE, LUA_TFUNCTION, LUA_TUSERDATA, LUA_TTHREAD, and LUA_TLIGHTUSERDATA.
    }
}

/// convenient way to call `llua_to_obj` which pops the stack.
// @within Converting
void *llua_to_obj_pop(lua_State *L, int idx) {
    void *res = llua_to_obj(L,idx);
    lua_pop(L,1);
    return res;
}

/// type name of the reference.
// @within Properties
const char *llua_typename(llua_t *o) {
    return lua_typename(o->L,o->type);
}

/// a reference to a new Lua table.
// @within Creating
llua_t *llua_newtable(lua_State *L) {
    llua_t *ref;
    lua_newtable(L);
    ref = llua_new(L,-1);
    lua_pop(L,1);
    return ref;
}

/// a reference to a C function.
// @within Creating
llua_t *llua_cfunction(lua_State *L, lua_CFunction f) {
    llua_t *ref;
    lua_pushcfunction(L,f);
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
// @within LoadingAndEvaluating
llua_t *llua_load(lua_State *L, const char *code, const char *name) {
    int res = luaL_loadbuffer(L,code,strlen(code),name);
    if (res != LUA_OK) {
        return (llua_t*)l_error(L);
   }
   return llua_to_obj_pop(L,-1);
}

/// load a file and return the compiled chunk as a reference.
// @within LoadingAndEvaluating
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

lua_State *_llua_push_nil(llua_t *o) {
    lua_State *L = llua_push(o);
    lua_pushnil(L);
    return L;
}

/// length of Lua reference, if a table or userdata.
// Note that we are specifically not using `lua_rawlen` here!
// @within Properties
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
// @within Converting
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
// @within Converting
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
// @within Converting
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

/// Read a value on the stack into a variable.
// `kind` is a  _type specifier_
//
//  * 'i' integer
//  * 'b' boolean
//  * 'f' double
//  * 's' string
//  * 'o' object (as in `llua_to_obj`)
//  * 'L' llua reference
//  * 'I' array of integers
//  * 'F' array of doubles
//  * 'S' array of strings
//
// @within Converting
err_t llua_convert(lua_State *L, char kind, void *P, int idx) {
    err_t err = NULL;
    switch(kind) {
    case 'i': // this is a tolerant operation; returns 0 if wrong type
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
        break;
    case 'L':
        *((llua_t**)P) = llua_new(L,idx);
        break;
    case 'F':
        if (! is_indexable(L,idx))
            err = "not indexable!*";
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
    if (err) {
        if (lua_isnil(L,idx))
            err = "was nil";
        return value_error(err);
    } else {
        return NULL;
    }
    
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
    case 'p':
        lua_pushlightuserdata(L,data);
        break;
    default:
        return value_error("unknown type");
    }
    return NULL;
}

/// call the reference, passing a number of arguments.
// These are specified by a set of _type specifiers_ `fmt`. Apart
// from the usual ones, we have 'm' (which must be first) which
// means "call the method by name" where `o` must be an object,
// 'v' which means "push the value at the index", and 'x' which
// means "C function".
//
// May optionally capture multiple return values with `llua_convert`.
// There are some common cases that have symbolic names:
//
//  * `L_NONE` call doesn't return anything
//  * `L_VAL`  we return a single value
//  * `L_REF`  we always return result as a llua reference
//  * `L_ERR`  Lua error convention, either <value> or <nil> <error-string>
//
// @within Calling
// @usage llua_callf(strfind,"ss",str,";$","i",&i);
// @usage llua_callf(open,"s","test.txt",L_ERR)
// @usage llua_callf(file,"ms","write","hello there\n",L_NONE);
void *llua_callf(llua_t *o, const char *fmt,...) {
    lua_State *L = o->L;
    int nargs = 0, nres = LUA_MULTRET, nerr;
    err_t res = NULL;
    char rtype;
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
        case 'v':
            lua_pushvalue(L,va_arg(ap,int) - nargs - 1);
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
                return (void*)llua_error(o,res);
            break;
        }
        ++fmt;
        ++nargs;
    }
    fmt = va_arg(ap,char*);
    if (fmt) {
        nres = strlen(fmt);
        rtype = *(fmt+1);
        if (*fmt == 'r' && rtype && rtype != 'E') // single return with explicit type
            nres = 1;
    }
    nerr = lua_pcall(L,nargs,nres,0);
    if (nerr != LUA_OK) {
        res = l_error(L);
    }
    if (nres == LUA_MULTRET || res != NULL) { // leave results on stack, or error!
        return (void*)llua_error(o,res);
    } else
    if (*fmt == 'r') { // return one value as object...
        if (rtype == 'E') {
            // Lua error return convention is object
            // or nil,error-string
            void *val = llua_to_obj(L,-2);
            void *err = llua_to_obj(L,-1);
            if (! val)
                val = value_error(err);
            lua_pop(L,2);
            return val;
        } else
        if (rtype) { // force the type!
            void *value;
            res = llua_convert(L,rtype,&value,-1);
            lua_pop(L,1);
            if (res) // failed...
                return (void*)llua_error(o,res);
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
            ++idx;
        }
        lua_pop(L,nres);
    }
    va_end(ap);
    return (void*)res;
}

/// call a function, raising an error.
// A useful combination of `llua_callf` and `llua_assert`
// @function llua_call_or_die
// @within Calling

/// pop some values off the Lua stack.
// Uses `llua_convert`
// @within Converting
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
// @within Properties
const char *llua_tostring(llua_t *o) {
    const char *res;
    llua_push(o);
    res = lua_tostring(o->L,-1);
    lua_pop(o->L,1);
    return res;
}

/// the Lua reference as a number.
// @within Properties
lua_Number llua_tonumber(llua_t *o) {
    lua_Number res;
    llua_push(o);
    res = lua_tonumber(o->L,-1);
    lua_pop(o->L,1);
    return res;
}

// can we index this object?
static bool accessible(llua_t *o, int ltype, const char *metamethod) {
    if (o->type == ltype) { // always cool
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
// @within Properties
bool llua_gettable(llua_t *o) {
    return accessible(o,LUA_TTABLE,"__index");
}

/// can we set a field of this object?
// @within Properties
bool llua_settable(llua_t *o) {
    return accessible(o,LUA_TTABLE,"__newindex");
}

/// can we call this object?
// @within Properties
bool llua_callable(llua_t *o) {
    return accessible(o,LUA_TFUNCTION,"__call");
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

// assume the table is initially on top of the stack.
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
// 'object' defined as with `llua_to_obj`
// @within GettingAndSetting
void *llua_gets(llua_t *o, const char *key) {
    lua_State *L = llua_push(o);
    safe_gets(L,key);
    lua_remove(L,-2);
    return llua_to_obj_pop(L,-1);
}

/// index the reference with multiple string keys and type-specifiers.
// Type specifiers are as with `llua_convert`
// @within GettingAndSetting
// @usage llua_gets_v(T,"key1","s",&str,NULL);
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
            char buff[256];
            snprintf(buff,sizeof(buff),"field '%s': %s",key,err);
            unref(err);
            err = value_error(buff);
            break;
        }
        key = va_arg(ap,const char*);
    }
    va_end(ap);
    lua_pop(L,1); // the reference
    return llua_error(o,err);
}

/// index the reference with an integer key.
// @within GettingAndSetting
void *llua_geti(llua_t *o, int key) {
    lua_State *L = llua_push(o);
    lua_pushinteger(L,key);
    lua_gettable(L,-2);
    lua_remove(L,-2); // the reference
    return llua_to_obj_pop(L,-1);
}

/// raw indexing with an integer key.
// @within GettingAndSetting
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
    }  else { // _probably_ a string. You have been warned...
        lua_pushstring(L,(const char*)value);
    }
}

/// set value using integer key.
// uses `llua_push_object`
// @within GettingAndSetting
void llua_seti(llua_t *o, int key, void *value) {
    lua_State *L = llua_push(o);
    lua_pushinteger(L,key);
    llua_push_object(L,value);
    lua_settable(L,-3);
}
/// set value using integer key.
// uses `llua_push_object`
// @within GettingAndSetting
void llua_sets(llua_t *o, const char *key, void *value) {
    lua_State *L = llua_push(o);
    llua_push_object(L,value);
    lua_setfield(o->L,-2,key);
}

// there's some code duplication here with llua_callf, but I'm not
// 100% sure how far to push <stdarg.h>!

/// set multiple keys and values on the reference.
// uses type specifiers like `llua_callf`
// @within GettingAndSetting
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
    return llua_error(o,err);
}

/// load and evaluate an expression.
// `fret` is a type specifier for the result, like `llua_callf`.
// @within LoadingAndEvaluating
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
// @within LoadingAndEvaluating
void *llua_evalfile(lua_State *L, const char *file, const char *fret, llua_t *env) {
    llua_t *chunk = llua_loadfile(L,file);
    if (value_is_error(chunk)) // compile failed...
        return llua_error(env,(err_t)chunk);
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
    return (void*) llua_error(env,res);
}


