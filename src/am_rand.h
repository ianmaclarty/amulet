// see am_rand.cpp
#define AM_RAND_KK 100
#define AM_RAND_QUALITY 1009

struct am_rand {
    tinymt32_t state;
    
    void init(int seed);

    double get_rand(); // returns a double in the range [0.0, 1.0).
    float get_randf(); // as above, but a float
};

am_rand* am_get_default_rand(lua_State *L);

void am_open_rand_module(lua_State *L);
