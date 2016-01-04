// see am_rand.cpp
#define AM_RAND_KK 100
#define AM_RAND_QUALITY 1009

struct am_rand {
    sfmt_t state;
    
    void init(int seed);

    double get_rand(); // returns a double in the range [0.0, 1.0).
};

void am_open_rand_module(lua_State *L);
