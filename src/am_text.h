struct am_font : am_userdata {
    FT_Face face;    
};

void am_open_text_module(lua_State *L);
