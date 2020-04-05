#include "amulet.h"
#ifdef AM_SPRITEPACK

#include "ft2build.h"
#include FT_FREETYPE_H

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define MAX_ITEMS 4096
#define MAX_TEX_SIZE 4096
#define ADVANCE_SCALE 0.015625f

#define NUMFMT "%.16g"

typedef struct {
    int is_font;
    int size;
    int *codepoints;
    char *filename;
    int face;
} item_spec;

typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} pixel;

static FT_Library ft_library;
static FT_Face ft_face;
static char *png_filename;
static char *lua_filename;
static item_spec items[MAX_ITEMS];
static int num_items = 0;
static stbrp_rect *rects = NULL;
static int num_rects = 0;
static int atlas_width = 128;
static int atlas_height = 64;
static unsigned char *atlas_data = NULL;
static unsigned char *image_data = NULL;
static unsigned char *bordered_image_data = NULL;
static int image_width = 0;
static int image_height = 0;
static int bordered_image_width = 0;
static int bordered_image_height = 0;
static int bb_width = 0;
static int bb_height = 0;
static int bb_left = 0;
static int bb_right = 0;
static int bb_top = 0;
static int bb_bottom = 0;
#define char_bitmap ft_face->glyph->bitmap
static int is_mono = 0;
static const char* minfilter = "linear";
static const char* magfilter = "linear";
static int do_premult = 1;
static int keep_padding = 0;
static int border = 1;
static int default_font = 0;

static void process_args(int argc, char *argv[]);
static void gen_rects();
static int try_pack();
static void premult();
static void write_data();
static void write_png();
static void load_font(int s);
static void load_char(int c);
static bool char_exists(int c);
static void load_image(int s);
static void compute_bordered_image();
static void compute_bbox();

bool am_pack_sprites(int argc, char *argv[]) {
    if (FT_Init_FreeType(&ft_library)) {
        fprintf(stderr, "error initializing freetype library\n");
        return false;
    }
    process_args(argc, argv);
    gen_rects();
    while (!try_pack()) {
        if (atlas_width == atlas_height) {
            atlas_width *= 2;
        } else {
            atlas_height = atlas_width;
        }
        if (atlas_width > MAX_TEX_SIZE) {
            fprintf(stderr, 
                "max texture size of %d exceeded while trying to pack font\n", MAX_TEX_SIZE);
            return false;
        }
    }
    atlas_data = (unsigned char*)malloc(4 * atlas_width * atlas_height);
    memset(atlas_data, 0, 4 * atlas_width * atlas_height);
    write_data();
    if (do_premult) {
        premult();
    }
    write_png();
    free(rects);
    free(atlas_data);
    FT_Done_Face(ft_face);
    return true;
}

static void gen_rects() {
    int s;
    int c;
    int r;
    num_rects = 0;
    for (s = 0; s < num_items; s++) {
        if (items[s].is_font) {
            load_font(s);
            c = 0;
            while (items[s].codepoints[c] >= 0) {
                if (char_exists(items[s].codepoints[c])) {
                    num_rects++;
                }
                c++;
            }
        } else {
            num_rects++;
        }
    }
    rects = (stbrp_rect*)malloc(sizeof(stbrp_rect) * num_rects);
    r = 0;
    for (s = 0; s < num_items; s++) {
        if (items[s].is_font) {
            load_font(s);
            c = 0;
            while (items[s].codepoints[c] >= 0) {
                int cp = items[s].codepoints[c];
                if (char_exists(cp)) {
                    load_char(cp);
                    rects[r].id = 0;
                    rects[r].w = char_bitmap.width + 2;
                    rects[r].h = char_bitmap.rows + 2;
                    r++;
                }
                c++;
            }
        } else {
            load_image(s);
            rects[r].id = 0;
            rects[r].w = bb_width + 2 * border;
            rects[r].h = bb_height + 2 * border;
            r++;
        }
    }
}

static int try_pack() {
    int r;
    stbrp_context ctx;
    stbrp_node *nodes = (stbrp_node*)malloc(atlas_width * sizeof(stbrp_node));
    stbrp_init_target(&ctx, atlas_width, atlas_height, nodes, atlas_width);
    stbrp_pack_rects(&ctx, rects, num_rects);
    free(nodes);
    for (r = 0; r < num_rects; r++) {
        if (!rects[r].was_packed) {
            return 0;
        }
    }
    return 1;
}

static void replace_backslashes(char *str) {
    for (unsigned int i = 0; i < strlen(str); i++) {
        if (str[i] == '\\') str[i] = '/';
    }
}

static void write_data() {
    int r = 0;
    int s;
    int c;
    int i, j;
    int rows;
    int width;
    int pitch;
    unsigned char *src_ptr;
    unsigned char *dest_ptr;
    double s1, t1, s2, t2, x1, y1, x2, y2;
    double w, h, adv;
    FILE *f = am_fopen(lua_filename, "w");
    if (f == NULL) {
        fprintf(stderr, "unable to open %s for writing: %s\n", lua_filename, strerror(errno));
        return;
    }
    fprintf(f, "local font_data = {\n");
    fprintf(f, "    minfilter = \"%s\",\n", minfilter);
    fprintf(f, "    magfilter = \"%s\",\n", magfilter);
    fprintf(f, "    is_premult = %s,\n", do_premult ? "true" : "false");
    for (s = 0; s < num_items; s++) {
        char *filename = am_format("%s", items[s].filename);
        replace_backslashes(filename);
        if (items[s].is_font) {
            int sz = items[s].size;
            load_font(s);
            fprintf(f, "    {\n");
            fprintf(f, "        is_font = true,\n");
            fprintf(f, "        size = %d,\n", sz);
            fprintf(f, "        mono = ");
            if (is_mono) {
                fprintf(f, "true");
            } else {
                fprintf(f, "false");
            }
            fprintf(f, ",\n");
            fprintf(f, "        filename = \"%s\",\n", filename);
            fprintf(f, "        face = %d,\n", items[s].face);
            fprintf(f, "        chars = (function()\n");
            fprintf(f, "            local chrs = {}\n");
            c = 0;
            while (items[s].codepoints[c] >= 0) {
                if (c % 256 == 0) {
                    if (c != 0) fprintf(f, "            end)();\n");
                    fprintf(f, "            (function()\n");
                }
                int cp = items[s].codepoints[c];
                if (!char_exists(cp)) {
                    c++;
                    continue;
                }
                load_char(cp);
                rows = char_bitmap.rows;
                width = char_bitmap.width;
                pitch = char_bitmap.pitch;
                src_ptr = char_bitmap.buffer;
                dest_ptr = atlas_data + (rects[r].x+1) * 4 +
                    (rects[r].y+1) * atlas_width * 4;
                if (char_bitmap.pixel_mode == FT_PIXEL_MODE_GRAY) {
                    for (i = 0; i < rows; i++) {
                        for (j = 0; j < width; j++) {
                            dest_ptr[j*4+0] = 0xFF;
                            dest_ptr[j*4+1] = 0xFF;
                            dest_ptr[j*4+2] = 0xFF;
                            dest_ptr[j*4+3] = src_ptr[j];
                        }
                        src_ptr += pitch;
                        dest_ptr += atlas_width * 4;
                    }
                } else if (char_bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
                    for (i = 0; i < rows; i++) {
                        for (j = 0; j < width; j++) {
                            dest_ptr[j*4+0] = 0xFF;
                            dest_ptr[j*4+1] = 0xFF;
                            dest_ptr[j*4+2] = 0xFF;
                            if (src_ptr[j>>3] & (128 >> (j&7))) {
                                dest_ptr[j*4+3] = 0xFF;
                            } else {
                                dest_ptr[j*4+3] = 0x00;
                            }
                        }
                        src_ptr += pitch;
                        dest_ptr += atlas_width * 4;
                    }
                } else {
                    fprintf(stderr, "unsupported pixel mode for codepoint %d: %d\n",
                        cp, char_bitmap.pixel_mode);
                    exit(EXIT_FAILURE);
                }

                s1 = ((double)rects[r].x) / (double)atlas_width;
                t2 = 1.0 - ((double)rects[r].y) / (double)atlas_height;
                w = ((double)rects[r].w) / (double)atlas_width;
                h = ((double)rects[r].h) / (double)atlas_height;
                t1 = t2 - h;
                s2 = s1 + w;

                x1 = (double)ft_face->glyph->bitmap_left - 1.0;
                y2 = (double)ft_face->glyph->bitmap_top + 2.0;
                w = (double)char_bitmap.width + 2.0;
                h = (double)char_bitmap.rows + 2.0;
                x2 = x1 + w;
                y1 = y2 - h;

                adv = (double)ft_face->glyph->advance.x * ADVANCE_SCALE;

                fprintf(f, 
                       "                chrs[%d] = {\n"
                       "                    x1 = " NUMFMT ", y1 = " NUMFMT ", x2 = " NUMFMT ", y2 = " NUMFMT ",\n"
                       "                    s1 = " NUMFMT ", t1 = " NUMFMT ", s2 = " NUMFMT ", t2 = " NUMFMT ",\n"
                       "                    advance = " NUMFMT ",\n"
                       "                }\n",
                    cp, 
                    x1, y1, x2, y2,
                    s1, t1, s2, t2,
                    adv);
                r++;
                c++;
            }
            fprintf(f, "            end)();\n");
            fprintf(f, "            return chrs\n");
            fprintf(f, "        end)(),\n");
            fprintf(f, "    },\n");
        } else {
            load_image(s);
            src_ptr = bordered_image_data + (bb_top - border) * bordered_image_width * 4 + (bb_left - border) * 4;
            dest_ptr = atlas_data + (rects[r].x) * 4 +
                (rects[r].y) * atlas_width * 4;
            for (i = 0; i < (bb_height + border * 2); i++) {
                memcpy(dest_ptr, src_ptr, (bb_width + border * 2) * 4);
                src_ptr += bordered_image_width * 4;
                dest_ptr += atlas_width * 4;
            }

            s1 = ((double)rects[r].x + 1.0 * (double)border) / (double)atlas_width;
            t2 = 1.0 - (((double)rects[r].y) + 1.0 * (double)border) / (double)atlas_height;
            w = ((double)bb_width) / (double)atlas_width;
            h = ((double)bb_height) / (double)atlas_height;
            t1 = t2 - h;
            s2 = s1 + w;

            x1 = (double)bb_left - 1.0 * (double)border;
            y1 = (double)(bordered_image_height - bb_bottom - (double)border * 2.0);
            w = (double)bb_width;
            h = (double)bb_height;
            x2 = x1 + w;
            y2 = y1 + h;

            fprintf(f, 
                   "    {\n"
                   "        filename = \"%s\",\n"
                   "        x1 = " NUMFMT ", y1 = " NUMFMT ", x2 = " NUMFMT ", y2 = " NUMFMT ",\n"
                   "        s1 = " NUMFMT ", t1 = " NUMFMT ", s2 = " NUMFMT ", t2 = " NUMFMT ",\n"
                   "        width = %d, height = %d,\n"
                   "    },\n",
                filename, 
                x1, y1, x2, y2,
                s1, t1, s2, t2,
                image_width, image_height);
            r++;
        }
        free(filename);
    }
    fprintf(f, "}\n\n");
    if (default_font) {
        fprintf(f, "am.default_font = am._init_fonts(font_data, \"lua/default_font.png\", true)\n");
    } else {
        fprintf(f, "return am._init_fonts(font_data, \"%s\")", png_filename);
    }
    fclose(f);
}

static void premult() {
    pixel *ptr = (pixel*)atlas_data;
    pixel *end = ptr + atlas_width * atlas_height;
    while (ptr != end) {
        int r = ptr->r;
        int g = ptr->g;
        int b = ptr->b;
        int a = ptr->a;
        ptr->r = (r * a + 1) >> 8;
        ptr->g = (g * a + 1) >> 8;
        ptr->b = (b * a + 1) >> 8;
        ptr++;
    }
}

static void write_png() {
    size_t len;
    void *png_data = tdefl_write_image_to_png_file_in_memory(
        atlas_data, atlas_width, atlas_height, 4, &len);
    FILE *f = am_fopen(png_filename, "wb");
    if (f == NULL) {
        fprintf(stderr, "unable to open %s for writing: %s\n", png_filename, strerror(errno));
        return;
    }
    fwrite(png_data, len, 1, f);
    fclose(f);
}

static int font_loaded = 0;

static void load_font(int s) {
    if (font_loaded) {
        FT_Done_Face(ft_face);
    }
    FT_Error err = FT_New_Face(ft_library, items[s].filename, items[s].face, &ft_face);
    if (err) {
        fprintf(stderr, "error loading font '%s': %d\n", items[s].filename, err);
        exit(EXIT_FAILURE);
    }
    if (FT_Set_Pixel_Sizes(ft_face, items[s].size, 0)) {
        fprintf(stderr, "unable to set size %d\n", items[s].size);
        exit(EXIT_FAILURE);
    }
    font_loaded = 1;
}

static bool char_exists(int c) {
    return c == ' ' || c == 0 || FT_Get_Char_Index(ft_face, c) > 0;
}

static void load_char(int c) {
    if (is_mono) {
        if (FT_Load_Char(ft_face, c, FT_LOAD_TARGET_MONO)) {
            fprintf(stderr, "unable to load codepoint %d\n", c);
            exit(EXIT_FAILURE);
        }
        if (FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_MONO)) {
            fprintf(stderr, "unable to render codepoint %d\n", c);
            exit(EXIT_FAILURE);
        }
    } else {
        if (FT_Load_Char(ft_face, c, FT_LOAD_RENDER)) {
            fprintf(stderr, "unable to load codepoint %d\n", c);
            exit(EXIT_FAILURE);
        }
    }
}

static void load_image(int s) {
    int components = 4;
    if (image_data != NULL) {
        free(image_data);
    }

    char *errmsg;
    int len;
    void *data = am_read_resource(items[s].filename, &len, &errmsg);
    if (data == NULL) {
        fprintf(stderr, "%s\n", errmsg);
        free(errmsg);
        exit(EXIT_FAILURE);
    }
    image_data =
        stbi_load_from_memory((stbi_uc const *)data, len, &image_width, &image_height, &components, 4);
    free(data);
    if (image_data == NULL) {
        fprintf(stderr, "error loading image file %s: %s\n", items[s].filename, stbi_failure_reason());
        exit(EXIT_FAILURE);
    }

    compute_bordered_image();
    compute_bbox();
}

static void compute_bordered_image() {
    int row;
    if (bordered_image_data != NULL) {
        free(bordered_image_data);
    }
    bordered_image_width = image_width + 2 * border;
    bordered_image_height = image_height + 2 * border;
    bordered_image_data = (unsigned char*)malloc(4 * bordered_image_width * bordered_image_height);
    uint32_t *img_ptr = (uint32_t*)image_data;
    uint32_t *bimg_ptr = (uint32_t*)bordered_image_data;
    for (row = 0; row < bordered_image_height; row++) {
        if (border && row == 0) {
            bimg_ptr[0] = img_ptr[0];
            memcpy(&bimg_ptr[1], &img_ptr[0], 4 * image_width);
            bimg_ptr[bordered_image_width-1] = img_ptr[image_width-1];
            bimg_ptr += bordered_image_width;
        } else if (border && row == bordered_image_height - 1) {
            img_ptr -= image_width;
            bimg_ptr[0] = img_ptr[0];
            memcpy(&bimg_ptr[1], &img_ptr[0], 4 * image_width);
            bimg_ptr[bordered_image_width-1] = img_ptr[image_width-1];
        } else {
            if (border) {
                bimg_ptr[0] = img_ptr[0];
            }
            memcpy(&bimg_ptr[border], &img_ptr[0], 4 * image_width);
            if (border) {
                bimg_ptr[bordered_image_width-1] = img_ptr[image_width-1];
            }
            bimg_ptr += bordered_image_width;
            img_ptr += image_width;
        }
    }
}

static void compute_bbox() {
    int row;
    int col;
    int row_clear = 1;
    int found_bb_top = 0;
    int w = image_width;
    int h = image_height;
    unsigned char alpha = 0;
    if (keep_padding) {
        bb_top = 0;
        bb_left = 0;
        bb_right = w - 1;
        bb_bottom = h - 1;
    } else {
        bb_top = h - 1;
        bb_left = w - 1;
        bb_right = 0;
        bb_bottom = 0;
        for (row = 0; row < h; row++) {
            row_clear = 1;
            for (col = 0; col < w; col++) {
                alpha = image_data[row * image_width * 4 + col * 4 + 3];
                if (alpha > 0) {
                    row_clear = 0;
                    if (col < bb_left) {
                        bb_left = col;
                    }
                    if (col > bb_right) {
                        bb_right = col;
                    }
                }
            }
            if (!row_clear) {
                if (!found_bb_top) {
                    bb_top = row;
                    found_bb_top = 1;
                }
                bb_bottom = row;
            }
        }
        if (bb_left > bb_right) {
            bb_right = bb_left;
        }
        if (bb_top > bb_bottom) {
            bb_top = bb_bottom;
        }
    }

    // make sure we have a 1 pixel transparent border, if possible
    if (border) {
        if (bb_left > 0) {
            bb_left = bb_left - 1;
        }
        if (bb_right < image_width - 1) {
            bb_right = bb_right + 1;
        }
        if (bb_top > 0) {
            bb_top = bb_top - 1;
        }
        if (bb_bottom < image_height - 1) {
            bb_bottom = bb_bottom + 1;
        }
    }

    // convert the bounding box to the bordered image coordinate space
    if (border) {
        bb_left = bb_left + 1;
        bb_top = bb_top + 1;
        bb_right = bb_right + 1;
        bb_bottom = bb_bottom + 1;
    }

    // compute bounding box dimensions
    bb_width = bb_right - bb_left + 1;
    bb_height = bb_bottom - bb_top + 1;
}

#define DEFAULT_START 0x20
#define DEFAULT_END 0x7E

static int read_char_spec(char **ptr) {
    int c;
    char *str = *ptr;
    int len = strlen(str);
    if (len > 2 && str[0] == '0' && str[1] == 'x') {
        c = strtol(str + 2, &str, 16);
    } else {
        c = str[0];
        str++;
    }
    *ptr = str;
    return c;
}

static char *read_range_spec(char *str, int *range_from, int *range_to) {
    int from, to;
    from = read_char_spec(&str);
    if (*str != '-') {
        fprintf(stderr, "invalid char range spec\n");
        exit(EXIT_FAILURE);
    }
    str++;
    if (*str == '\0') {
        fprintf(stderr, "invalid char range spec\n");
        exit(EXIT_FAILURE);
    }
    to = read_char_spec(&str);
    *range_from = from;
    *range_to = to;
    return str;
}

static int *read_ranges(char *str) {
    char *ptr = str;
    int from, to;
    int count = 0;
    int i, j;
    int has_space = 0;
    while (1) {
        ptr = read_range_spec(ptr, &from, &to);
        if (to < from) {
            fprintf(stderr, "invalid range spec: %x-%x\n", from, to);
            exit(EXIT_FAILURE);
        }
        has_space = has_space || (from <= ' ' && ' ' <= to);
        count += to - from + 1;
        if (*ptr == '\0') {
            break;
        }
        if (*ptr == ',') {
            ptr++;
        } else {
            fprintf(stderr, "unexpected range separator: %c\n", *ptr);
            exit(EXIT_FAILURE);
        }
    }
    int *ranges = (int*)malloc((count + 1 + (has_space ? 0 : 1)) * sizeof(int));
    if (has_space) {
        ranges[count] = -1;
    } else {
        ranges[count] = ' ';
        ranges[count + 1] = -1;
    }
    ptr = str;
    i = 0;
    while (1) {
        ptr = read_range_spec(ptr, &from, &to);
        for (j = from; j <= to; j++) {
            ranges[i++] = j;
        }
        if (*ptr == '\0') break;
        ptr++;
    }
    return ranges;
}

static void parse_spec(char *arg, int s) {
    // font.ttf#1@16:A-Z,a-z,0-9,0x20-0x2F,0x3A-0x40,0x5B-0x60,0x7B-0x7E
    // or
    // img.png
    int i;
    items[s].filename = arg;
    int len = strlen(arg);
    if (len > 4 && 
        (  strcmp(arg + len - 4, ".png") == 0
        || strcmp(arg + len - 4, ".jpg") == 0
        || strcmp(arg + len - 4, ".tga") == 0
        || strcmp(arg + len - 4, ".bmp") == 0
        || strcmp(arg + len - 4, ".psd") == 0
        || strcmp(arg + len - 4, ".gif") == 0
        || strcmp(arg + len - 4, ".hdr") == 0
        || strcmp(arg + len - 4, ".pic") == 0
        || strcmp(arg + len - 4, ".ppm") == 0
        || strcmp(arg + len - 4, ".pgm") == 0
        ))
    {
        items[s].is_font = 0;
        return; // nothing else to parse
    }
    items[s].is_font = 1;
    // parse font spec
    items[s].face = 0;
    while (*arg != '\0' && *arg != '@' && *arg != '#') arg++;
check_size:
    switch (*arg) {
        case '\0':
            fprintf(stderr, "no size specified for font '%s'\n", items[s].filename);
            exit(EXIT_FAILURE);
            return;
        case '#':
            *arg = '\0';
            items[s].face = strtol(arg + 1, &arg, 10) - 1;
            if (items[s].face < 0) {
                fprintf(stderr, "font face must be positive\n");
                exit(EXIT_FAILURE);
            }
            goto check_size;
        case '@':
            *arg = '\0';
            items[s].size = strtol(arg + 1, &arg, 10);
            break;
        default:
            fprintf(stderr, "unexpected character: '%c'\n", *arg);
            exit(EXIT_FAILURE);
            return;
    }
    if (*arg == '\0') {
        items[s].codepoints = (int*)malloc(sizeof(int) * (DEFAULT_END - DEFAULT_START + 2));
        for (i = 0; i <= (DEFAULT_END - DEFAULT_START); i++) {
            items[s].codepoints[i] = i + DEFAULT_START;
        }
        items[s].codepoints[DEFAULT_END - DEFAULT_START + 1] = -1;
        return;
    }
    if (*arg != ':') {
        fprintf(stderr, "unexpected character: '%c' (expecting ':')\n", *arg);
        exit(EXIT_FAILURE);
    }
    arg++;
    items[s].codepoints = read_ranges(arg);
}

static void usage() {
    fprintf(stderr, "Usage: amulet pack -png filename.png -lua filename.lua [-mono] [-minfiler <filter>] [-magfilter <filter>] [-no-premult] [-keep-padding] <spec> ...\n");
    fprintf(stderr, "  where <spec> is either an image file or a font spec of the form:\n");
    fprintf(stderr, "  font.ttf@16[:A-Z,0x20-0x2F]\n");
    exit(EXIT_FAILURE);
}

static void process_args(int argc, char *argv[]) {
    int a;
    if (argc < 5) usage();

    png_filename = NULL;
    lua_filename = NULL;
    for (a = 0; a < argc; ++a) {
        const char *arg = argv[a];
        if (strcmp(arg, "-png") == 0) {
            if (++a >= argc) usage();
            png_filename = argv[a];
        } else if (strcmp(arg, "-lua") == 0) {
            if (++a >= argc) usage();
            lua_filename = argv[a];
        } else if (strcmp(arg, "-mono") == 0) {
            is_mono = 1;
        } else if (strcmp(arg, "-default") == 0) {
            default_font = 1;
        } else if (strcmp(arg, "-minfilter") == 0) {
            if (++a >= argc) usage();
            minfilter = argv[a];
        } else if (strcmp(arg, "-magfilter") == 0) {
            if (++a >= argc) usage();
            magfilter = argv[a];
        } else if (strcmp(arg, "-no-premult") == 0) {
            do_premult = 0;
        } else if (strcmp(arg, "-keep-padding") == 0) {
            keep_padding = 1;
        } else if (strcmp(arg, "-no-border") == 0) {
            border = 0;
        } else {
            parse_spec(argv[a], num_items);
            num_items++;
        }
    }
    if (png_filename == NULL) usage();
    if (lua_filename == NULL) usage();
    if (num_items == 0) usage();
}
#endif
