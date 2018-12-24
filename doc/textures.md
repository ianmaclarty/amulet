
# Image buffers {#image-buffers}

An image buffer represents a 2D image in memory.
Each pixel occupies 4 bytes with 1 byte per channel (RGBA).
The data for the image buffer is stored in an [`am.buffer`](#am.buffer).

## Creating image buffers

### am.image_buffer([buffer, ] width [, height]) {#am.image_buffer .func-def}

Creates an image buffer of the given width and height.
If `height` is omitted it is the same as `width` (the
image is square).

If a `buffer` (created using [`am.buffer`](#am.buffer))
is given, it is used as the image buffer. It must have
the correct size, which is `4 * width * height`.
If `buffer` is omitted a new one is created.

Fields:

- `width`: the image width.
- `height`: the image height.
- `buffer`: the raw data buffer.

### am.load_image(filename) {#am.load_image .func-def}

Loads the given image file and returns a new image buffer.
Only `.png` and `.jpg` files are supported.
Returns `nil` if the file was not found.

## Saving images

### image_buffer:save_png(filename) {#image_buffer:save_png .method-def}

Saves the given image as a png in `filename`.

## Pasting images

### image_buffer:paste(src, x, y) {#image_buffer:paste .method-def}

Pastes one image into another such that the bottom-left corner
of the source image is at the given pixel coordinate in the target image.
The bottom-left pixel of the target image has coordinate (1, 1).

## Decoding/encoding PNGs

### am.encode_png(image_buffer) {#am.encode_png .func-def}

Returns a raw buffer containing the png encoding of the given
image.

### am.decode_png(buffer) {#am.decode_png .func-def}

Converts the raw buffer, which should be a png encoding
of an image, into an image buffer.

# Textures {#textures}

Textures are 2D images that can be used as input to shader programs
when bound to a `sampler2D` uniform.

They can also be rendered to via [framebuffers](#am.framebuffer).

A texture may optionally have a backing [image buffer](#image-buffers).
If a texture has a backing image buffer, then any changes to the image
buffer will be automatically transferred to the texture.

Textures always have 4 channels (RGBA) with 1 byte per channel.

## Creating a texture

### am.texture2d(width [, height]) {#am.texture2d .func-def}

Creates a texture of the given width and height without a backing
image buffer.

### am.texture2d(image_buffer) { .func-def}

Creates a texture using the given image buffer as the backing
image buffer.

### am.texture2d(filename) { .func-def}

This is shorthand for `am.texture2d(am.load_image(filename))`.

## Texture fields

### texture.width {#texture.width .field-def}

Readonly.

### texture.height {#texture.height .field-def}

Readonly.

### texture.image_buffer {#texture.image_buffer .field-def}

The backing image buffer or `nil` if there isn't one.

Readonly.

### texture.minfilter {#texture.minfilter .field-def}

Defines the minification filter which is applied when the texture's pixels
are smaller that the screen's pixels.

The allowed values are:

- `"nearest"` (the default)
- `"linear"`
- `"nearest_mipmap_nearest"`
- `"linear_mipmap_nearest"`
- `"nearest_mipmap_linear"`
- `"linear_mipmap_linear"`

If one of the mipmap filters is used a mipmap will be automatically generated.

Updatable.

### texture.magfilter {#texture.magfilter .field-def}

Defines the magnification filter which is applied when the texture's pixels
are larger that the screen's pixels.

The allowed values are:

- `"nearest"` (the default)
- `"linear"`

Updatable.

### texture.filter {#texture.filter .field-def}

This field can be used to set both the minification and magnification
fields. The allowed values are:

- `"nearest"`
- `"linear"`

Updatable.

### texture.swrap {#texture.swrap .field-def}

Sets the wrapping mode in the x direction. The allowed values are:

- `"clamp_to_edge"` (the default)
- `"repeat"`
- `"mirrored_repeat"`

Note that the texture width must be a power of 2 if either
`"repeat"` or `"mirrored_repeat"` is used.

Updatable.

### texture.twrap {#texture.twrap .field-def}

Sets the wrapping mode in the y direction. The allowed values are:

- `"clamp_to_edge"` (the default)
- `"repeat"`
- `"mirrored_repeat"`

Note that the texture height must be a power of 2 if either
`"repeat"` or `"mirrored_repeat"` is used.

Updatable.

### texture.wrap {#texture.wrap .field-def}

This field can be used to set the wrap mode in both the
x and y directions at the same time.

The allowed values are:

- `"clamp_to_edge"`
- `"repeat"`
- `"mirrored_repeat"`

Updatable.

# Framebuffers

A framebuffer is like an off-screen window you can draw to.
It has a texture associated with it that gets updated when you
draw to the framebuffer. The texture can then be used as input to
further rendering.

## Creating a framebuffer

### am.framebuffer(texture [, depth_buf [, stencil_buf]]) {#am.framebuffer .func-def}

Creates framebuffer with the given texture attached.

`depth_buf` and `stencil_buf` determine whether the
framebuffer has a depth and/or stencil buffer. These should be
`true` or `false` (default is `false`).

## Framebuffer fields

### framebuffer.clear_color {#framebuffer.clear_color .field-def}

The color to use when clearing the framebuffer (a `vec4`).

Updatable.

### framebuffer.stencil_clear_value {#framebuffer.stencil_clear_value .field-def}

The value to use when clearing the framebuffer's stencil buffer (an integer between 0 and 255).

Updatable.

### framebuffer.projection {#framebuffer.projection .field-def}

A `mat4` projection matrix to use when rendering nodes into
this framebuffer. The `"P"` uniform will be set to this.
If this is not specified then the projection
`math.ortho(-width/2, width/2, -height/2, height/2)` is used.

Updatable.

### framebuffer.pixel_width {#framebuffer.pixel_width .field-def}

The width of the framebuffer, in pixels.

Readonly.

### framebuffer.pixel_height {#framebuffer.pixel_height .field-def}

The height of the framebuffer, in pixels.

Readonly.

## Framebuffer methods

### framebuffer:render(node) {#framebuffer:render .method-def}

Renders `node` into the framebuffer.

### framebuffer:render_children(node) {#framebuffer:render_children .method-def}

Renders `node`'s children into the framebuffer (but not `node` itself).

### framebuffer:clear([color [, depth [, stencil]]]) {#framebuffer:clear .method-def}

Clears the framebuffer, using the current [`clear_color`](#framebuffer.clear_color).

By default the color, depth and stencil buffers are
cleared, but you can selectively clear only some buffers by
setting the `color`, `depth` and `stencil` argumnets to `true`
or `false`.

### framebuffer:read_back() {#framebuffer:read_back .method-def}

Reads the pixels from the framebuffer into the texture's
backing image buffer. This is a relatively slow operation,
so use it sparingly.

### framebuffer:resize(width, height) {#framebuffer:resize .method-def}

Resize the framebuffer. Only framebuffers with textures that have
no backing image buffers can be resized. Also the texture must not
use a filter that requires a mipmap.
