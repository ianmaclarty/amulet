
# Packing sprites and generating fonts {#spritepack}

## Overview

Amulet includes a tool for packing images and font glyphs
into a sprite sheet and generating a Lua module for 
conveniently accessing the images and glyphs therein.

Suppose you have an `images` directory containing some
`.png` files and a `fonts` directory containing `myfont.ttf`
and suppose these two directories are subdirectories
of the main game directory (where your `main.lua` file
lives). To generate a sprite sheet,
run the following command while in the main game
directory:

~~~ {.console}
> amulet pack -png mysprites.png -lua mysprites.lua 
              images/*.png fonts/myfont.ttf@32
~~~

This will generate `mysprites.png` and `mysprites.lua`
in the current directory.

The `@32` after the font file specifies the size of the
glyphs to generate (in this case 32px).

To use the generated sprite sheet in your code, first
required the Lua module and then pass the fields in that
module to `am.sprite` or `am.text`. For example:

~~~ {.lua}
local mysprites = require "mysprites"
local run1_node = am.sprite(mysprites.run1)
local text_node = am.text(mysprites.myfont32, "BARF!")
~~~

The sprite field names are generated from the image filenames
with the extension removed (so the file `images/run1.png`
would result in the field `mysprites.run1`).

The font field name (`myfont32`) is the concatenation
of the font file name (without the extension) and the
font size.

## Pack options

In addition to the required `-png` and `-lua` options the
pack command also supports the following options:

Option                     Description
------------------------   --------------------------------------------------------------------------------------------------------
`-mono`                    Do not anti-alias fonts.
`-minfilter`               The minification filter to apply when loading the sprite sheet texture. `linear` (the default) or `nearest`.
`-magfilter`               The magnification filter to apply when loading the sprite sheet texture. `linear` (the default) or `nearest`.
`-no-premult`              Do not pre-multiply RGB channels by alpha.
`-keep-padding`            Do not strip transparent pixels around images.

## Padding

Unless you specify the `-keep-padding` option, Amulet will
remove excess rows and columns of completely transparent pixels
from each image before packing it. It will always however leave
a one pixel border of transparent pixels if the image was surrounded
by transparent pixels to begin with. This is done to prevent
pixels from one image "bleeding" into another
image when it's drawn.

When excess rows and columns are removed the vertices of the sprite will be
adjusted so the images is still drawn in the correct position, as if the
excess pixels were still there.

If an image is not surrounded by a border of transparent pixels, then
the border pixels will be duplicated around the image. This helps
prevent "cracks" when tiling the image.

In summary, if you don't intend to use an image for tiling,
surround it with a border of completely transparent
pixels. 

## Specifying which font glyphs to generate

Optionally each font item may also be followed by a colon and a comma
separated list of character ranges to include in the sprite sheet.

The characters in the character range can be written directly if they
are ASCII, or given as hexadecimal Unicode codepoints.

Here are some examples of valid font items:

- `VeraMono.ttf@32:A-Z,0-9`
- `ComicSans.ttf@122:0x20-0xFF`
- `gbsn00lp.ttf@42:a-z,A-Z,0-9,0x4E00-0x9FFF`

If the character range list is omitted, it defaults to `0x20-0x7E`,
i.e: 

~~~ {.text}
!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~0123456789
ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz
~~~

## Loading bitmap font images

### am.load_bitmap_font(filename, key) {#am.load_bitmap_font .func-def}

Loads the image `filename` and returns a font using
the glyph layout described by `key` where `key` is a string
containing all the glyphs in the file as they are layed out.
All glyphs must have the same width and height which is
determined from the width and height of the image and the
number of rows and columns in `key`. For example if the
key is:

~~~ {.lua}
[[
ABC
DEF
GHI
]]
~~~

then the image contains 9 glyphs in 3 rows and 3 columns.
If the image has height 30 and width 36 then each glyph has
width 10 and height 12.
