local img1 = am.load_image("../logo.png")
local pngbuf1 = am.encode_png(img1)
local base64 = am.base64_encode(pngbuf1)
local pngbuf2 = am.base64_decode(base64)
local img2 = am.decode_png(pngbuf2)
local view1 = img1.buffer:view("ubyte", 0, 1)
local view2 = img2.buffer:view("ubyte", 0, 1)
assert(#view1 == #view2)
for i = 1, #view1 do
    assert(view1[i] == view2[i])
end
print("ok")
