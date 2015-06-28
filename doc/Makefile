include ../Common.mk

LUA_HTML_MANUAL = ../$(LUA_DIR)/doc/manual.html

SECTIONS = lua_builtins.txt \
	vecmat.txt

manual.html: $(SECTIONS) writer.lua style.css template.html Makefile
	cat $(SECTIONS) | pandoc -s --template template.html -c style.css --toc -f markdown -o $@

clean:
	rm manual.html
