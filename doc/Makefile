include ../Common.mk

LUA_HTML_MANUAL=../$(LUA_DIR)/doc/manual.html

manual.html: gen_api_ref.lua $(LUA_HTML_MANUAL) functions.lua
	cat header.html > $@
	../$(BUILD_BIN_DIR)/amulet$(EXE_EXT) gen_api_ref.lua $(LUA_HTML_MANUAL) >> $@
	cat footer.html >> $@

clean:
	rm manual.html
