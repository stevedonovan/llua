#ifdef LLUA_H
#define LLUA_H
#else
#include <stdio.h>
#include <llib/obj.h>
#include <llib/value.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

typedef const char *err_t;

typedef struct LLua_ {
    lua_State *L;
    int ref;
    int type;
} llua_t;

// useful names for common function returns
#define L_VAL "r"
#define L_REF "rL"
#define L_ERR "rE"
#define L_NONE ""

// the FOR_TABLE construct
#define L_TKEY (-2)
#define L_TVAL (-1)
#define FOR_TABLE(t) \
 for (lua_State *_L=_llua_push_nil(t);lua_next(_L,-2) != 0;lua_pop(_L,1))
#define llua_table_break(t) {lua_pop(_L,1);break;}


llua_t *llua_new(lua_State *L, int idx);
void llua_verbose(FILE *f);
bool llua_is_lua_object(llua_t *o);
llua_t* llua_global(lua_State *L);
void *llua_to_obj(lua_State *L, int idx);
void *llua_to_obj_pop(lua_State *L, int idx);
llua_t *llua_getmetatable(llua_t *o);
void llua_setmetatable(llua_t *o, llua_t *mt);
const char *llua_typename(llua_t *o);
llua_t *llua_newtable(lua_State *L);
llua_t *llua_load(lua_State *L, const char *code, const char *name);
llua_t *llua_loadfile(lua_State *L, const char *filename);
lua_State *llua_push(llua_t *o);
lua_State *_llua_push_nil(llua_t *o);
int llua_len(llua_t *o);
err_t llua_call(llua_t *o, int nargs, int nresults);
double *llua_tonumarray(lua_State* L, int idx);
int *llua_tointarray(lua_State* L, int idx);
char** llua_tostrarray(lua_State* L, int idx);
err_t llua_convert(lua_State *L, char kind, void *P, int idx);
void *llua_callf(llua_t *o, const char *fmt,...);
err_t llua_pop_vars(lua_State *L, const char *fmt,...);
const char *llua_tostring(llua_t *o);
lua_Number llua_tonumber(llua_t *o);
bool llua_gettable(llua_t *o);
bool llua_settable(llua_t *o);
void *llua_gets(llua_t *o, const char *key);
err_t llua_gets_v(llua_t *o, const char *key,...);
void *llua_geti(llua_t *o, int key);
void *llua_rawgeti(llua_t* o, int key);
void llua_push_object(lua_State *L, void *value);
void llua_seti(llua_t *o, int key, void *value);
void llua_sets(llua_t *o, const char *key, void *value);
err_t llua_sets_v(llua_t *o, const char *key,...);
void *llua_eval(lua_State *L, const char *expr, const char *fret);
void *llua_evalfile(lua_State *L, const char *file, const char *fret, llua_t *env);
#endif
