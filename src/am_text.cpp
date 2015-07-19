#include "amulet.h"

#define ADVANCE_SCALE 0.015625f

static FT_Library ft_library;
static bool ft_initialized = false;

static void ensure_ft_init() {
    if (!ft_initialized) {
        if (FT_Init_FreeType(&ft_library)) {
            am_abort("error initializing freetype library");
        }
        ft_initialized = true;
    }
}

// copied from lua-5.3.0:
#define MAXUNICODE      0x10FFFF
/*
** Decode one UTF-8 sequence, returning NULL if byte sequence is invalid.
*/
static const char *utf8_decode (const char *o, int *val) {
  static unsigned int limits[] = {0xFF, 0x7F, 0x7FF, 0xFFFF};
  const unsigned char *s = (const unsigned char *)o;
  unsigned int c = s[0];
  unsigned int res = 0;  /* final result */
  if (c < 0x80)  /* ascii? */
    res = c;
  else {
    int count = 0;  /* to count number of continuation bytes */
    while (c & 0x40) {  /* still have continuation bytes? */
      int cc = s[++count];  /* read next byte */
      if ((cc & 0xC0) != 0x80)  /* not a continuation byte? */
        return NULL;  /* invalid byte sequence */
      res = (res << 6) | (cc & 0x3F);  /* add lower 6 bits from cont. byte */
      c <<= 1;  /* to test next bit */
    }
    res |= ((c & 0x7F) << (count * 5));  /* add first byte */
    if (count > 3 || res > MAXUNICODE || res <= limits[count])
      return NULL;  /* invalid byte sequence */
    s += count;  /* skip continuation bytes read */
  }
  if (val) *val = res;
  return (const char *)s + 1;  /* +1 to include first byte */
}

static int load_font(lua_State *L) {
    am_check_nargs(L, 1);
    const char *name = luaL_checkstring(L, 1);
    int len;
    char *errmsg;
    void *data = am_read_resource(name, &len, &errmsg);
    if (data == NULL) {
        lua_pushstring(L, errmsg);
        free(errmsg);
        return lua_error(L);
    }
    am_font *font = am_new_userdata(L, am_font);
    font->name = name;
    font->ref(L, 1); // keep ref to name
    font->data = data;
    if (FT_New_Memory_Face(ft_library, (const FT_Byte*)data, len, 0, &font->face)) {
        return luaL_error(L, "error reading font '%s'", name);
    }
    return 1;
}

static int font_gc(lua_State *L) {
    am_font *font = am_get_userdata(L, am_font, 1);
    FT_Done_Face(font->face);
    free(font->data);
    font->data = NULL;
    return 0;
}

static void draw_bitmap(FT_Bitmap *bitmap, float leftf, float topf, am_image *img) {
    int bleft = (int)floorf(leftf+0.5f);
    int btop = (int)floorf(topf+0.5f);
    int bwidth = bitmap->width;
    int bheight = bitmap->rows;
    int bpitch = bitmap->pitch;
    unsigned char *bptr = bitmap->buffer;
    unsigned char bmode = bitmap->pixel_mode;
    int iwidth = img->width;
    uint8_t *ibuffer = img->buffer->data;
    int isize = img->buffer->size;

    if (bmode != FT_PIXEL_MODE_GRAY) {
        am_log1("WARNING: unsupported pixel mode %d in glyph bitmap (glyph will not be rendered)", bmode);
        return;
    }

    switch (img->format) {
        case AM_PIXEL_FORMAT_RGBA8: {
            int ioffset;
            for (int row = 0; row < bheight; row++) {
                ioffset = (btop - row) * iwidth * 4 + bleft * 4;
                if (ioffset >= isize || ioffset < 0) continue;
                for (int i = 0; i < bwidth && bleft + i < iwidth; i++) {
                    unsigned char value = bptr[i];
                    ibuffer[ioffset++] = value;
                    ibuffer[ioffset++] = value;
                    ibuffer[ioffset++] = value;
                    ibuffer[ioffset++] = value;
                }
                bptr += bpitch;
            }
            int start_offset = (btop - bheight + 1) * iwidth * 4 + bleft * 4;
            int end_offset = btop * iwidth * 4 + (bleft + bwidth) * 4;
            if (start_offset < isize && end_offset > 0) {
                start_offset = am_clamp(start_offset, 0, isize-1);
                end_offset = am_clamp(end_offset, 1, isize);
                img->buffer->mark_dirty(start_offset, end_offset);
            }
            break;
        }
        case AM_PIXEL_FORMAT_LUM8: {
            // XXX NYI
            break;
        }
    }
}

static int render_text(lua_State *L) {
    int nargs = am_check_nargs(L, 5);
    am_font *font = am_get_userdata(L, am_font, 1);
    am_image *img = am_get_userdata(L, am_image, 2);
    glm::vec2 pen = am_get_userdata(L, am_vec2, 3)->v;
    size_t n;
    const char *text = luaL_checklstring(L, 4, &n);
    int width = luaL_checkinteger(L, 5);
    int height = 0;
    if (width <= 0) {
        return luaL_error(L, "expecting a positive integer in position 5 (in fact %d)", width);
    }
    if (nargs > 5) {
        height = luaL_checkinteger(L, 6);
        if (height <= 0) {
            return luaL_error(L, "expecting a positive integer in position 6 (in fact %d)", height);
        }
    }
    if (FT_Set_Pixel_Sizes(font->face, width, height)) {
        if (nargs > 5) {
            return luaL_error(L, "size %dx%d is not supported by font '%s'", width, height, font->name);
        } else {
            return luaL_error(L, "size %d is not supported by font '%s'", width, font->name);
        }
    }
    FT_Face face = font->face;
    FT_GlyphSlot slot = face->glyph;
    const char *ptr = text;
    const char *end = text + n;
    while (ptr < end) {
        int c = 0;
        ptr = utf8_decode(ptr, &c);
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            am_log1("WARNING: error loading font codepoint %u", c);
            continue;
        }
        float left = pen.x + (float)slot->bitmap_left;
        float top = pen.y + (float)slot->bitmap_top;
        draw_bitmap(&slot->bitmap, left, top, img);
        pen.x += ((float)slot->advance.x) * ADVANCE_SCALE;
    }
    return 0;
}

static int render_codepoint(lua_State *L) {
    int nargs = am_check_nargs(L, 5);
    am_font *font = am_get_userdata(L, am_font, 1);
    am_image *img = am_get_userdata(L, am_image, 2);
    glm::vec2 pen = am_get_userdata(L, am_vec2, 3)->v;
    int32_t codepoint = luaL_checkinteger(L, 4);
    int width = luaL_checkinteger(L, 5);
    int height = 0;
    if (width <= 0) {
        return luaL_error(L, "expecting a positive integer in position 5 (in fact %d)", width);
    }
    if (nargs > 5) {
        height = luaL_checkinteger(L, 6);
        if (height <= 0) {
            return luaL_error(L, "expecting a positive integer in position 6 (in fact %d)", height);
        }
    }
    if (FT_Set_Pixel_Sizes(font->face, width, height)) {
        if (nargs > 5) {
            return luaL_error(L, "size %dx%d is not supported by font '%s'", width, height, font->name);
        } else {
            return luaL_error(L, "size %d is not supported by font '%s'", width, font->name);
        }
    }
    FT_Face face = font->face;
    FT_GlyphSlot slot = face->glyph;
    if (FT_Load_Char(face, codepoint, FT_LOAD_RENDER)) {
        am_log1("WARNING: error loading font codepoint %u", codepoint);
        return 0;
    }
    float left = pen.x + (float)slot->bitmap_left;
    float top = pen.y + (float)slot->bitmap_top;
    draw_bitmap(&slot->bitmap, left, top, img);
    return 0;
}

static void register_font_mt(lua_State *L) {
    lua_newtable(L);

    am_set_default_index_func(L);
    am_set_default_newindex_func(L);
    lua_pushcclosure(L, font_gc, 0);
    lua_setfield(L, -2, "__gc");

    lua_pushcclosure(L, render_text, 0);
    lua_setfield(L, -2, "render_text");
    lua_pushcclosure(L, render_codepoint, 0);
    lua_setfield(L, -2, "render_codepoint");

    am_register_metatable(L, "font", MT_am_font, 0);
}

static int read_utf8_codepoint(lua_State *L) {
    am_check_nargs(L, 2);
    size_t len;
    const char *str = luaL_checklstring(L, 1, &len);
    int pos = luaL_checkinteger(L, 2);
    if (pos > (int)len || pos < 1) {
        lua_pushnil(L);
        return 1;
    }
    int codepoint;
    int next;
    const char *ptr = str + (pos-1);
    const char *res = utf8_decode(ptr, &codepoint);
    if (res == NULL) {
        codepoint = 0; // invalid UTF-8 byte sequence
        next = pos + 1;
    } else {
        next = pos + (res - ptr);
    }
    lua_pushinteger(L, codepoint);
    lua_pushinteger(L, next);
    return 2;
}

void am_open_text_module(lua_State *L) {
    ensure_ft_init();
    luaL_Reg funcs[] = {
        {"load_font", load_font},
        {"_read_utf8_codepoint", read_utf8_codepoint},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_font_mt(L);
}
