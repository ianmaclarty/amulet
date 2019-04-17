extern bool am_record_perf_timings;

extern double am_last_frame_lua_time;
extern double am_last_frame_draw_time;
extern double am_last_frame_audio_time;

void am_open_time_module(lua_State *L);
