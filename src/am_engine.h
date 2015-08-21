struct am_engine {
    am_allocator *allocator;
    lua_State *L;
    bool worker;
};

am_engine *am_init_engine(bool worker, int argc, char** argv);
void am_destroy_engine(am_engine *eng);
