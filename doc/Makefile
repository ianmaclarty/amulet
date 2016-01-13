include ../Common.mk

LUA_HTML_MANUAL = ../$(LUA51_DIR)/doc/contents.html

SECTIONS = header.md intro.md lua.md quickstart.md math.md buffers.md scene_nodes.md graphics.md actions.md input.md audio.md conf.md api_ref.md

GRAPHS = $(wildcard graphs/*.dot)
GRAPH_PNGS = $(patsubst %.dot,%.png,$(GRAPHS))

ifndef VERSION
VERSION=DEV
endif

index.html: $(SECTIONS) $(GRAPH_PNGS) style.css template.html Makefile 
	cat $(SECTIONS) | pandoc --template template.html -V amulet-version="$(VERSION)" -c style.css --toc -f markdown -o $@

$(GRAPH_PNGS): %.png: %.dot
	dot -Tpng $< -o $@

clean:
	rm -f index.html
	rm -f graphs/*.png
