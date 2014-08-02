void am_set_globals_metatable(lua_State *L);
void am_requiref(lua_State *L, const char *modname, lua_CFunction openf);
void am_open_module(lua_State *L, const char *name, luaL_Reg *funcs);

void am_register_metatable(lua_State *L, int metatable_id, int parent_mt_id);
void am_push_metatable(lua_State *L, int metatable_id);
void am_set_metatable(lua_State *L, int metatable_id, int idx);
void *am_check_metatable_id(lua_State *L, int metatable_id, int idx);
int am_get_metatable_id(lua_State *L, int idx);

void *am_new_nonatomic_userdata(lua_State *L, size_t sz);
int am_new_ref(lua_State *L, int from, int to);
void am_delete_ref(lua_State *L, int obj, int ref);
void am_push_ref(lua_State *L, int obj, int ref);
void am_replace_ref(lua_State *L, int obj, int ref, int new_val);

int am_check_nargs(lua_State *L, int n);

struct am_property {
    void (*getter) (lua_State*, void*);
    void (*setter) (lua_State*, void*);
};

void am_register_property(lua_State *L, const char *field, const am_property *property);

void am_set_default_index_func(lua_State *L);
void am_set_default_newindex_func(lua_State *L);

bool am_call(lua_State *L, int nargs, int nresults);

// The caller must ensure v is live and was created
// with lua_newuserdata.
void lua_unsafe_pushuserdata(lua_State *L, void *v);

#ifdef AM_LUAJIT
void lua_setuservalue(lua_State *L, int idx);
void lua_getuservalue(lua_State *L, int idx);
int lua_rawlen(lua_State *L, int idx);
lua_Integer lua_tointegerx(lua_State *L, int idx, int *isnum);
#endif

#define am_absindex(L, idx) ((idx) > 0 ? (idx) : lua_gettop(L) + (idx) + 1)

template<typename T>
struct am_lua_vector {
    T *arr;
    int size;
    int capacity;
    int ref;

    am_lua_vector() {
        arr = NULL;
        size = 0;
        capacity = 0;
        ref = LUA_NOREF;
    }

    void push_back(lua_State *L, int owner, T val) {
        ensure_capacity(L, owner, size + 1);
        arr[size++] = val;
    }

    void push_front(lua_State *L, int owner, T val) {
        ensure_capacity(L, owner, size + 1);
        for (int i = size; i > 0; i--) {
            arr[i] = arr[i-1];
        }
        arr[0] = val;
        size++;
    }

    void remove(int index) {
        if (index >= size) return;
        for (int i = index; i < size - 1; i++) {
            arr[i] = arr[i+1];
        }
        size--;
    }

    void clear() {
        size = 0;
    }

    void ensure_capacity(lua_State *L, int owner, int c) {
        if (capacity < c) {
            owner = am_absindex(L, owner);
            int old_capacity = capacity;
            if (capacity == 0) capacity = 1;
            while (capacity < c) capacity *= 2;
            T *new_arr = (T*)lua_newuserdata(L, sizeof(T) * capacity);
            if (old_capacity > 0) {
                memcpy(new_arr, arr, sizeof(T) * old_capacity);
                am_replace_ref(L, owner, ref, -1);
            } else {
                ref = am_new_ref(L, owner, -1);
            }
            arr = new_arr;
            lua_pop(L, 1); // new_arr
        }
    }
};
