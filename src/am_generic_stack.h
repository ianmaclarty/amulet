struct am_generic_stack_slot {
    int version;
    int prev_offset;
    struct { union { double d; void *p; }; } data[];
};

template<typename T>
struct am_generic_stack {
    char *base;
    am_generic_stack_slot *top;
    char *end;
    int capacity;

    am_generic_stack() {
        base = NULL;
        top = NULL;
        end = NULL;
        capacity = 0;
    }

    virtual ~am_generic_stack() {
        if (base != NULL) free(base);
    }

    inline void grow_by(int amount) {
        int old_size = ((char*)end - base);
        int new_size = old_size + amount;
        if (capacity < new_size) {
            int top_offset = ((char*)top - base);
            if (capacity == 0) capacity = 8;
            while (capacity < new_size) capacity *= 2;
            base = (char*)realloc(base, capacity);
            top = (am_generic_stack_slot*)(base + top_offset);
            end = base + old_size;
        }
    }

    void *push(int version, int sz) {
        int slot_size = sizeof(am_generic_stack_slot) + sz;
        grow_by(slot_size);
        int prev_offset = (char*)top - base;
        top = (am_generic_stack_slot*)end;
        end = end + slot_size;
        top->version = version;
        top->prev_offset = prev_offset;
        return (void*)&top->data[0];
    }

    inline void *push_copy(int version) {
        assert(!is_empty());
        int slot_size = end - (char*)top;
        grow_by(slot_size);
        memcpy(end, top, slot_size);
        top = (am_generic_stack_slot*)end;
        end += slot_size;
        top->version = version;
        top->prev_offset += slot_size;
        return (void*)&top->data[0];
    }

    T *push_copy_if_new_version(int version) {
        assert(!is_empty());
        assert(version >= top->version);
        if (version > top->version) {
            return push_copy(version);
        } else {
            return (T*)&top->data[0];
        }
    }

    T *get_top() {
        assert(!is_empty());
        return (T*)&top->data[0];
    }

    inline void pop() {
        assert(!is_empty());
        end = (char*)top;
        top = (am_generic_stack_slot*)(base + top->prev_offset);
    }

    void rollback(int version) {
        assert(!is_empty());
        assert(version >= top->version);
        if (version == top->version) {
            pop();
            assert(is_empty() || version > top->version);
        }
    }

    bool is_empty() {
        return end == base;
    }
};
