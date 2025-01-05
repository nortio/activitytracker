NAME=activitytracker
DOMAIN=nortio.github.io
#pack
.PHONY: all install clean process

all: dist/extension.js

dist/extension.js: extension.ts | node_modules
	./node_modules/esbuild/bin/esbuild extension.ts --target=firefox115 --outdir=dist --bundle --format=esm --external:gettext --external:"gi://*" --external:"resource://*"

node_modules:
	npm install

#schemas/gschemas.compiled: schemas/org.gnome.shell.extensions.$(NAME).gschema.xml
#	glib-compile-schemas schemas

#$(NAME).zip: dist/extension.js dist/prefs.js #schemas/gschemas.compiled
#	@cp -r schemas dist/
#	@cp metadata.json dist/
#	@(cd dist && zip ../$(NAME).zip -9r .)

#pack: $(NAME).zip

install: all
	@cp metadata.json dist/
	@touch ~/.local/share/gnome-shell/extensions/$(NAME)@$(DOMAIN)
	@rm -rf ~/.local/share/gnome-shell/extensions/$(NAME)@$(DOMAIN)
	@mv dist ~/.local/share/gnome-shell/extensions/$(NAME)@$(DOMAIN)

reporter: reporter-cpp/main.cpp
	c++ -std=c++20 reporter-cpp/main.cpp -o reporter -O3

clean:
	@rm -rf dist node_modules $(NAME).zip
