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
    // Lua error convention: value or nil, errstr
    llua in = llua_callf(G("io.open"),"ss",file,"rb",L_ERR);
    if (value_is_error(in)) {
        fprintf(stderr,"error: %s\n",in);
        return 1;
    }
    llua text = llua_callf(in,"ms","read","*a",L_REF);
    llua_callf(in,"m","close","");

    // text here is a reference to a Lua string,
    // can use llua_tostring() to get pointer.
    // Note this works with an arbitrary binary file!
    printf("size of %s was %d\n",file,llua_len(text));

    // first ten words in the text
    llua iter = llua_callf(G("string.gmatch"),"os",text,"%a%a%a+",L_VAL);
    FOR(i,10) {
        char* res = llua_callf(iter,L_NONE,L_VAL);
        printf("string %s\n",res);
    }


    unref(P); // all refs are freed...
    //~ size of c:\Users\steve\dev\llua\file-size.exe was 45056
    //~ string This
    //~ string program
    //~ string cannot
    //~ string run
    //~ string DOS
    //~ string mode
    //~ string text
    //~ string data
    //~ string rdata
    //~ string bss
    //~ free L 0000000000306B80 ref 3 type table
    //~ free L 0000000000306B80 ref 4 type function
    //~ free L 0000000000306B80 ref 5 type userdata
    //~ free L 0000000000306B80 ref 6 type string
    //~ free L 0000000000306B80 ref 7 type function
    //~ free L 0000000000306B80 ref 8 type function
    lua_close(L);
}
