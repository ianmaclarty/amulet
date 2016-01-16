
Buffers and views {#buffers-and-views}
=================

Buffers are contiguous blocks of memory. They are used for storing images,
audio and vertex data, or anything else you like.

You can't access a buffer's memory directly.  Instead you access a buffer
through a *view*.  Views provide a typed array-like interface to the buffer.

### am.buffer(size) {#am.buffer .func-def}

Returns a new buffer of the given size in bytes.

The buffer's memory will zeroed.

The `#` operator can be used to retrieve the size of a buffer in bytes.

### buffer:view(type [, offset [, stride [, count]]]) {#buffer:view .func-def}

Returns a view into `buffer`.

`type` can be one of the following:

type                    size (bytes)  Lua value range             internal range            endianess
--------------------   -------------  --------------------------- ------------------------  -----------
`"float"`                          4  approx -3.4e38 to 3.4e38    same                      native
`"vec2"`                           8  any `vec2`                  same                      native
`"vec3"`                          12  any `vec3`                  same                      native
`"vec4"`                          16  any `vec4`                  same                      native
`"byte"`                           1  -128 to 127                 same                      N/A
`"ubyte"`                          1  0 to 255                    same                      N/A
`"byte_norm"`                      1  -1.0 to 1.0                 -127 to 127               N/A
`"ubyte_norm"`                     1  0.0 to 1.0                  0 to 255                  N/A
`"short"`                          2  -32768 to 32767             same                      native
`"ushort"`                         2  0 to 65535                  same                      native
`"short_norm"`                     2  -1.0 to 1.0                 -32767 to 32767           native
`"ushort_norm"`                    2  0.0 to 1.0                  0 to 65535                native
`"ushort_elem"`                    2  1 to 65536                  0 to 65535                native
`"int"`                            4  -2147483648 to 2147483647   same                      native
`"uint"`                           4  0 to 4294967295             same                      native
`"uint_elem"`                      4  1 to 4294967296             0 to 4294967295           native

The `_norm` types map Lua numbers in the range -1 to 1
(or 0 to 1 for unsigned types) to integer values in the buffer.

The `_elem` types are specifically for element array buffers and
offset the Lua numbers by 1 to conform to the Lua convension of 
array indices starting at 1.

All view types currently use the native platform endianess, which happens
to be little-endian on all currently supported platforms.

The `offset` argument is the byte offset of the first element of the
view. The default is 0.

The `stride` argument is the distance between consecutive values in the
view, in bytes. The default is the size of the view type.

The `count` argument determines the number of elements in the view.
The underlying buffer must be large enough accomodate the
elements with the given stride. The default is the maximum supported by the
buffer with the given stride.

You can read and write to views as if they were Lua arrays (as with Lua
arrays, indices start at 1). For example:

~~~ {.lua}
local buf = am.buffer(12)
local view = buf:view("float")
view[1] = 1.5 
view[2] = view[1] + 2
~~~

Attempting to read an index less than 1 or larger than the number of elements
will return nil.

You can retrieve the number of elements in a view using the `#` operator.

### view:slice(n [, count]) {#view:slice .func-def}

Returns a new view of the same type as `view` that references the same buffer,
but which starts at the `n`th element of `view` and continues for
`count` elements. If `count` is ommitted it is `#view - n + 1` (i.e.
it covers all the elements of `view` after and including the `n`th).

### view:set(val [, count]) {#view:set .func-def}

Bulk sets values in a view. This is faster than setting them
one at a time.

If `val` is a number or vector then this sets up to `count` elements
of `view` to `val`. If `count` is ommitted all the elements are set.

If `val` is a table then the elements are set to their corresponding
values from the table. If `count` is given then at most `count` elements
are set (fewer may be set if the table has less than `count` elements).

As a special case, if `val` is a table of numbers and the view's type
is a vector, then the elements of the table will be used to set the
components of the vectors in the view. For example:

~~~ {.lua}
local verts = am.buffer(32):view("vec3")
verts:set{1, 2, 3, 4, 5, 6}
print(verts[1]) -- vec3(1, 2, 3)
print(verts[2]) -- vec3(4, 5, 6)
~~~

Finally if `val` is another view then the elements are set to the
corresponding values from that view. The views may be of different
types as long as they are "compatible". The types are converted
as if each element were set using the Lua code `view1[i] = view2[i]`.
This means you can't set a number view to a vector view or
vice versa.

### am.float_array(table) {#am.float_array .func-def}

Returns a float view to a newly created buffer containing
all the given values. `values` 

### am.byte_array(values) {#am.byte_array .func-def}
### am.ubyte_array(values) {#am.ubyte_array .func-def}
### am.byte_norm_array(values) {#am.byte_norm_array .func-def}
### am.ubyte_norm_array(values) {#am.ubyte_norm_array .func-def}
### am.short_array(values) {#am.short_array .func-def}
### am.ushort_array(values) {#am.ushort_array .func-def}
### am.short_norm_array(values) {#am.short_norm_array .func-def}
### am.ushort_norm_array(values) {#am.ushort_norm_array .func-def}
### am.int_array(values) {#am.int_array .func-def}
### am.uint_array(values) {#am.uint_array .func-def}
### am.int_norm_array(values) {#am.int_norm_array .func-def}
### am.uint_norm_array(values) {#am.uint_norm_array .func-def}
### am.ushort_elem_array(values) {#am.ushort_elem_array .func-def}
### am.uint_elem_array(values) {#am.uint_elem_array .func-def}
### am.vec2_array(values) {#am.vec2_array .func-def}
### am.vec3_array(values) {#am.vec3_array .func-def}
### am.vec4_array(values) {#am.vec4_array .func-def}

### am.load_buffer(filename) {#am.load_buffer .func-def}
### am.base64_encode(buffer) {#am.base64_encode .func-def}
### am.base64_decode(string) {#am.base64_decode .func-def}
