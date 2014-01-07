MANPAGES = $(wildcard xml/*.?.xml)
DEBUG=

html/%.html : xml/%.xml
	@echo Building $@
	@$(DEBUG) xmlto html-nochunks xml/$*.xml
	@$(DEBUG) mv -f `basename $*`.html html/

man/%: xml/%.xml
	@echo Building $@
	@$(DEBUG) xmlto man xml/`basename $*`.xml
	@$(DEBUG) mv -f `basename $*` man/

all: manpages htmldocs

manpages: $(patsubst xml/%.xml,man/%,$(MANPAGES))

htmldocs: $(patsubst xml/%.xml,html/%.html,$(MANPAGES))
