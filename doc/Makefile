include ../Common.mk

LUA_HTML_MANUAL = ../$(LUA51_DIR)/doc/manual.html

SECTIONS = header.md intro.md lua.md quickstart.md math.md buffers.md graphics.md actions.md input.md audio.md conf.md api_ref.md

GRAPHS = $(wildcard graphs/*.dot)
GRAPH_PNGS = $(patsubst %.dot,%.png,$(GRAPHS))

manual.html: $(SECTIONS) $(GRAPH_PNGS) writer.lua style.css template.html Makefile 
	cat $(SECTIONS) | pandoc -s --template template.html -c style.css --toc -f markdown -o $@

$(GRAPH_PNGS): %.png: %.dot
	dot -Tpng $< -o $@

clean:
	rm manual.html
