include ../Common.mk

LUA_HTML_MANUAL = ../$(LUA51_DIR)/doc/contents.html

SECTIONS = header.md \
	intro.md \
	lua.md \
	quickstart.md \
	math.md \
	buffers.md \
	windows.md \
	scene_nodes.md \
	programs.md \
	textures.md \
	actions.md \
	time.md \
	save.md \
	audio.md \
	controller.md \
	spritepack.md \
	export.md \
	conf.md \
	models.md \
	table.md \
	types.md \
	random.md \
	require.md \
	misc.md


GRAPHS = $(wildcard graphs/*.dot)
GRAPH_PNGS = $(patsubst %.dot,%.png,$(GRAPHS))

index.html: $(SECTIONS) $(GRAPH_PNGS) style.css template.html Makefile
	cat $(SECTIONS) | pandoc --mathjax --template template.html -c style.css --toc --toc-depth=4 -f markdown -o $@

$(GRAPH_PNGS): %.png: %.dot
	dot -Tpng $< -o $@

clean:
	rm -f index.html
	rm -f graphs/*.png
