win = am.window{}

ib1 = am.image_buffer(1)
v1 = ib1.buffer:view"ubyte"
tx1 = am.texture2d(ib1)
fb1 = am.framebuffer(tx1)

fb1:clear()
fb1:read_back()
assert(v1[1] == 0)
assert(v1[2] == 0)
assert(v1[3] == 0)
assert(v1[4] == 255)
fb1.clear_color = vec4(128, 64, 32, 16) / 255
fb1:clear()
fb1:read_back()
assert(v1[1] == 128)
assert(v1[2] == 64)
assert(v1[3] == 32)
assert(v1[4] == 16)

fb1.clear_color = vec4(0, 0, 0, 1)

fb1:clear()
fb1:render(am.rect(-2, -2, 2, 2, vec4(0, 1, 0, 1)))
fb1:read_back()
assert(v1[1] == 0)
assert(v1[2] == 255)
assert(v1[3] == 0)
assert(v1[4] == 255)

fb1:clear()
fb1:render(am.rect(-2, -2, 2, 2, vec4(255, 255, 0, 128) / 255))
fb1:read_back()
assert(v1[1] == 128)
assert(v1[2] == 128)
assert(v1[3] == 0)
assert(v1[4] == 64 + 255 - 128)

fb1:clear()
fb1:render(am.circle(vec2(0, 0), 2, vec4(255, 255, 255, 64) / 255))
fb1:read_back()
assert(v1[1] == 64)
assert(v1[2] == 64)
assert(v1[3] == 64)
assert(v1[4] == 16 + 255 - 64)

fb1:clear()
fb1:render(am.line(vec2(-2, -2), vec2(2, 2), 4, vec4(255, 0, 128, 128) / 255))
fb1:read_back()
assert(v1[1] == 128)
assert(v1[2] == 0)
assert(v1[3] == 64)
assert(v1[4] == 64 + 255 - 128)

fb1:clear()
fb1:render(am.scale(4) ^ am.text("I", vec4(255, 0, 0, 128) / 255))
fb1:read_back()
assert(v1[1] == 128)
assert(v1[2] == 0)
assert(v1[3] == 0)
assert(v1[4] == 128 + 255 - 128)

fb1:clear()
fb1:render(am.sprite("CCC\nCCC", vec4(128, 128, 128, 128) / 255))
fb1:read_back()
assert(v1[1] == 0)
assert(v1[2] == 64)
assert(v1[3] == 64)
assert(v1[4] == 64 + 255 - 128)

win:close()
print"ok"
