struct am_font : am_nonatomic_userdata {
    FT_Face face;    
    const char *name;
};

void am_open_text_module(lua_State *L);
