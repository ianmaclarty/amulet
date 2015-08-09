#include <stdio.h>

#include "ft2build.h"
#include FT_FREETYPE_H

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "miniz.c"

#define MAX_ITEMS 4096
#define MAX_TEX_SIZE 4096
#define ADVANCE_SCALE 0.015625f

typedef struct {
    int is_font;
    int size;
    int *codepoints;
    char *filename;
    int face;
} item_spec;

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
static int image_width = 0;
static int image_height = 0;
static int bb_width = 0;
static int bb_height = 0;
static int bb_left = 0;
static int bb_right = 0;
static int bb_top = 0;
static int bb_bottom = 0;
#define char_bitmap ft_face->glyph->bitmap
static int is_mono = 0;

static void process_args(int argc, char *argv[]);
static void gen_rects();
static int try_pack();
static void write_data();
static void write_png();
static void load_font(int s);
static void load_char(int c);
static void load_image(int s);
static void compute_bbox();

int main(int argc, char *argv[]) {
    if (FT_Init_FreeType(&ft_library)) {
        fprintf(stderr, "error initializing freetype library\n");
        return EXIT_FAILURE;
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
            return EXIT_FAILURE;
        }
    }
    atlas_data = malloc(4 * atlas_width * atlas_height);
    memset(atlas_data, 0, 4 * atlas_width * atlas_height);
    write_data();
    write_png();
    free(rects);
    free(atlas_data);
    FT_Done_Face(ft_face);
    return EXIT_SUCCESS;
}

static void gen_rects() {
    int s;
    int c;
    int r;
    num_rects = 0;
    for (s = 0; s < num_items; s++) {
        if (items[s].is_font) {
            c = 0;
            while (items[s].codepoints[c] >= 0) {
                num_rects++;
                c++;
            }
        } else {
            num_rects++;
        }
    }
    rects = malloc(sizeof(stbrp_rect) * num_rects);
    r = 0;
    for (s = 0; s < num_items; s++) {
        if (items[s].is_font) {
            int sz = items[s].size;
            load_font(s);
            c = 0;
            while (items[s].codepoints[c] >= 0) {
                load_char(items[s].codepoints[c]);
                rects[r].id = 0;
                rects[r].w = char_bitmap.width + 1;
                rects[r].h = char_bitmap.rows + 1;
                r++;
                c++;
            }
        } else {
            load_image(s);
            rects[r].id = 0;
            rects[r].w = bb_width + 1;
            rects[r].h = bb_height + 1;
            r++;
        }
    }
}

static int try_pack() {
    int r;
    stbrp_context ctx;
    stbrp_node *nodes = malloc(atlas_width * sizeof(stbrp_node));
    stbrp_init_target(&ctx, atlas_width-1, atlas_height-1, nodes, atlas_width);
    stbrp_pack_rects(&ctx, rects, num_rects);
    free(nodes);
    for (r = 0; r < num_rects; r++) {
        if (!rects[r].was_packed) {
            return 0;
        }
    }
    return 1;
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
    double s1, t1, s2, t2, x1, y1, x2, y2, w, h, adv;
    FILE *f = fopen(lua_filename, "w");
    fprintf(f, "local font_data = {\n");
    for (s = 0; s < num_items; s++) {
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
            fprintf(f, "        filename = \"%s\",\n", items[s].filename);
            fprintf(f, "        face = %d,\n", items[s].face);
            fprintf(f, "        chars = {\n");
            c = 0;
            while (items[s].codepoints[c] >= 0) {
                int cp = items[s].codepoints[c];
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
                            /*
                            dest_ptr[j*4+0] = src_ptr[j];
                            dest_ptr[j*4+1] = src_ptr[j];
                            dest_ptr[j*4+2] = src_ptr[j];
                            dest_ptr[j*4+3] = 0xFF;
                            */
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
                            /*
                            dest_ptr[j*4+0] = src_ptr[j];
                            dest_ptr[j*4+1] = src_ptr[j];
                            dest_ptr[j*4+2] = src_ptr[j];
                            dest_ptr[j*4+3] = 0xFF;
                            */
                        }
                        src_ptr += pitch;
                        dest_ptr += atlas_width * 4;
                    }
                } else {
                    fprintf(stderr, "unsupported pixel mode for codepoint %d: %d\n",
                        cp, char_bitmap.pixel_mode);
                    exit(EXIT_FAILURE);
                }

                s1 = ((double)rects[r].x + 0.5) / (double)atlas_width;
                t2 = 1.0 - (((double)rects[r].y) + 0.5) / (double)atlas_height;
                w = ((double)rects[r].w) / (double)atlas_width;
                h = ((double)rects[r].h) / (double)atlas_height;
                t1 = t2 - h;
                s2 = s1 + w;
                x1 = (double)ft_face->glyph->bitmap_left + 0.5;
                y2 = (double)ft_face->glyph->bitmap_top - 0.5;
                w = (double)char_bitmap.width;
                h = (double)char_bitmap.rows;
                x2 = x1 + w;
                y1 = y2 - h;
                adv = (double)ft_face->glyph->advance.x * ADVANCE_SCALE;
                fprintf(f, 
                       "            [%d] = {\n"
                       "                x1 = %g, y1 = %g, x2 = %g, y2 = %g,\n"
                       "                s1 = %g, t1 = %g, s2 = %g, t2 = %g,\n"
                       "                advance = %g,\n"
                       "            },\n",
                    cp, 
                    x1, y1, x2, y2,
                    s1, t1, s2, t2,
                    adv);
                r++;
                c++;
            }
            fprintf(f, "        }\n");
            fprintf(f, "    },\n");
        } else {
            load_image(s);
            src_ptr = image_data + bb_top * image_width * 4 + bb_left * 4;
            dest_ptr = atlas_data + (rects[r].x+1) * 4 +
                (rects[r].y+1) * atlas_width * 4;
            for (i = 0; i < bb_height; i++) {
                memcpy(dest_ptr, src_ptr, bb_width * 4);
                src_ptr += image_width * 4;
                dest_ptr += atlas_width * 4;
            }
            s1 = ((double)rects[r].x + 0.5) / (double)atlas_width;
            t2 = 1.0 - (((double)rects[r].y) + 0.5) / (double)atlas_height;
            w = ((double)rects[r].w) / (double)atlas_width;
            h = ((double)rects[r].h) / (double)atlas_height;
            t1 = t2 - h;
            s2 = s1 + w;
            x1 = (double)bb_left + 0.5;
            y1 = (double)(image_height - bb_bottom) + 0.5;
            w = (double)bb_width;
            h = (double)bb_height;
            x2 = x1 + w;
            y2 = y1 + h;
            fprintf(f, 
                   "    {\n"
                   "        filename = \"%s\",\n"
                   "        x1 = %g, y1 = %g, x2 = %g, y2 = %g,\n"
                   "        s1 = %g, t1 = %g, s2 = %g, t2 = %g,\n"
                   "    },\n",
                items[s].filename, 
                x1, y1, x2, y2,
                s1, t1, s2, t2);
            r++;
        }
    }
    fprintf(f, "}\n\n");
    fprintf(f, "return amulet._init_fonts(font_data, \"%s\")", png_filename);
    fclose(f);
}

static void write_png() {
    size_t len;
    void *png_data = tdefl_write_image_to_png_file_in_memory(
        atlas_data, atlas_width, atlas_height, 4, &len);
    FILE *f = fopen(png_filename, "wb");
    fwrite(png_data, len, 1, f);
    fclose(f);
}

static int font_loaded = 0;

static void load_font(int s) {
    if (font_loaded) {
        FT_Done_Face(ft_face);
    }
    if (FT_New_Face(ft_library, items[s].filename, items[s].face, &ft_face)) {
        fprintf(stderr, "error loading font '%s'\n", items[s].filename);
        exit(EXIT_FAILURE);
    }
    if (FT_Set_Pixel_Sizes(ft_face, items[s].size, 0)) {
        fprintf(stderr, "unable to set size %d\n", items[s].size);
        exit(EXIT_FAILURE);
    }
    font_loaded = 1;
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
    image_data =
        stbi_load(items[s].filename, &image_width, &image_height, &components, 4);
    if (image_data == NULL) {
        fprintf(stderr, "error loading image file %s: %s\n", items[s].filename, stbi_failure_reason());
        exit(EXIT_FAILURE);
    }
    compute_bbox();
}

static void compute_bbox() {
    int row;
    int col;
    int row_clear = 1;
    int found_bb_top = 0;
    int w = image_width;
    int h = image_height;
    unsigned char alpha = 0;
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
    bb_width = bb_right - bb_left + 1;
    bb_height = bb_bottom - bb_top + 1;
}

#define DEFAULT_START 0x20
#define DEFAULT_END 0x7E

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
        items[s].codepoints = malloc(sizeof(int) * (DEFAULT_END - DEFAULT_START + 3));
        for (i = 0; i <= (DEFAULT_END - DEFAULT_START); i++) {
            items[s].codepoints[i] = i + DEFAULT_START;
        }
        items[s].codepoints[DEFAULT_END - DEFAULT_START + 1] = 0;
        items[s].codepoints[DEFAULT_END - DEFAULT_START + 2] = -1;
        return;
    }
    if (*arg != ':') {
        fprintf(stderr, "unexpected character: '%c' (expecting ':')\n", *arg);
        exit(EXIT_FAILURE);
    }
}

static void process_args(int argc, char *argv[]) {
    /* myfont.png myfont.lua font.ttf@16:A-Z,a-z,0-9,0x20-0x2F,0x3A-0x40,0x5B-0x60,0x7B-0x7E */

    int i;
    if (argc < 4) {
        fprintf(stderr, "expecting at least 3 arguments\n");
        exit(EXIT_FAILURE);
    }

    png_filename = argv[1];
    lua_filename = argv[2];
    num_items = argc-3;
    for (i = 0; i < num_items; i++) {
        parse_spec(argv[i+3], i);
    }
}
