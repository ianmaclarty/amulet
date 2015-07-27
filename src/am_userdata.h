/*
 * Memory management notes
 * -----------------------
 * Instances of am_userdata and am_nonatomic_userdata are managed by the Lua
 * GC.
 * They are allocated as Lua userdata values.
 * am_nonatomic_userdata objects may contain references to other Lua managed
 * objects.
 * These references are maintained in the userdata's uservalue table so they
 * can be traced by the GC.
 */

#define am_new_userdata(L, T, ...) \
    ((T*)am_init_userdata(L, new (lua_newuserdata(L, sizeof(T))) T(__VA_ARGS__), MT_##T))

#define am_get_userdata(L, T, idx) \
    ((T*)am_check_metatable_id(L, MT_##T, idx))

#define am_get_userdata_or_nil(L, T, idx) \
    (lua_isnil(L, idx) ? NULL : ((T*)am_check_metatable_id(L, MT_##T, idx)))

#define AM_METATABLE_ID_MASK 0x0000FFFF
#define AM_NONATOMIC_MASK    0x00010000

template<typename T>
struct am_lua_array;

struct am_userdata {
    uint32_t ud_flags;

    am_userdata();

    // push the Lua userdata for this object onto the stack
    void push(lua_State *L);

    virtual void force_vtable() {};
};

struct am_nonatomic_userdata : am_userdata {
    int num_refs; // number of references from this object to other
                  // lua objects. A value of -1 indicates that the
                  // uservalue table for this object has not been
                  // created yet.
    int freelist; // head of freelist. 0 if empty.

    am_nonatomic_userdata();

    // Create, delete, replace references to other Lua values
    int ref(lua_State *L, int idx);
    void unref(lua_State *L, int ref);
    void reref(lua_State *L, int ref, int idx);

    // Push referenced object onto the stack.
    void pushref(lua_State *L, int ref);

    void pushuservalue(lua_State *L);
};

template<typename T>
struct am_lua_array {

    T *arr;
    T one;
    int size;
    int capacity;
    int ref;
    am_nonatomic_userdata *owner;

    am_lua_array() {
        arr = NULL;
        size = 0;
        capacity = 0;
        ref = LUA_NOREF;
        owner = NULL;
    }

    void push_back(lua_State *L, T val) {
        ensure_capacity(L, size + 1);
        arr[size++] = val;
    }

    void push_front(lua_State *L, T val) {
        insert(L, 0, val);
    }

    void insert(lua_State *L, int pos, T val) {
        assert(pos >= 0);
        assert(pos <= size);
        ensure_capacity(L, size + 1);
        for (int i = size; i > pos; i--) {
            arr[i] = arr[i-1];
        }
        arr[pos] = val;
        size++;
    }

    void remove(int index) {
        if (index >= size) return;
        for (int i = index; i < size - 1; i++) {
            arr[i] = arr[i+1];
        }
        size--;
    }

    bool remove_all(T elem) {
        int i = 0;
        int j = 0;
        bool found = false;
        while (j < size) {
            if (i < j) {
                arr[i] = arr[j];
            }
            if (arr[i] != elem) {
                i++;
            } else {
                found = true;
            }
            j++;
        }
        size = i;
        return found;
    }

    bool remove_first(T elem) {
        for (int i = 0; i < size; i++) {
            if (arr[i] == elem) {
                remove(i);
                return true;
            }
        }
        return false;
    }

    void clear() {
        size = 0;
    }

    inline void ensure_capacity(lua_State *L, int c) {
        assert(owner != NULL);
        if (capacity < c) {
            if (capacity == 0 && c == 1) {
                arr = &one;
                capacity = 1;
            } else {
                int old_capacity = capacity;
                if (capacity == 0) capacity = 1;
                while (capacity < c) capacity *= 2;
                T *new_arr = (T*)lua_newuserdata(L, sizeof(T) * capacity);
                if (old_capacity > 0) {
                    memcpy(new_arr, arr, sizeof(T) * old_capacity);
                }
                if (ref == LUA_NOREF) {
                    ref = owner->ref(L, -1);
                } else {
                    owner->reref(L, ref, -1);
                }
                arr = new_arr;
                lua_pop(L, 1); // new_arr
            }
        }
    }
};

struct am_property {
    void (*getter) (lua_State*, void*);
    void (*setter) (lua_State*, void*);
};

void am_register_metatable(lua_State *L, const char *tname, int metatable_id, int parent_mt_id);
void am_push_metatable(lua_State *L, int metatable_id);
int am_get_type(lua_State *L, int idx);
const char* am_get_typename(lua_State *L, int metatable_id);
am_userdata *am_check_metatable_id(lua_State *L, int metatable_id, int idx);
am_userdata *am_init_userdata(lua_State *L, am_userdata *ud, int metatable_id);

void am_register_property(lua_State *L, const char *field, const am_property *property);

int am_default_index_func(lua_State *L);
int am_default_newindex_func(lua_State *L);
void am_set_default_index_func(lua_State *L);
void am_set_default_newindex_func(lua_State *L);
bool am_try_default_newindex(lua_State *L);

void am_open_userdata_module(lua_State *L);
