include ../Common.mk

LUA_HTML_MANUAL=../$(LUA_DIR)/doc/manual.html

api_ref.html: gen_api_ref.lua $(LUA_HTML_MANUAL) functions.lua
	../amulet gen_api_ref.lua $(LUA_HTML_MANUAL) > $@

clean:
	rm api_ref.html
