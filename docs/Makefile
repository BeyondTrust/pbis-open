MANPAGES = $(wildcard md/*.?.md)
DEBUG=

man/%: md/%.md
	@echo Building $@
	@$(DEBUG) awk -v enterprise=0 -f filter-enterprise.awk md/$*.md | sed -e "/\[EDITION\]/ {s//Open/}" | ronn -r > md/$*
	@$(DEBUG) sed -e "/^.P$$/ {N; s/.P\n\[INDENT\]/.IP\n/}" md/$* > $@
	rm md/$*
# Generate an HTML version of the page
#	@$(DEBUG) awk -v enterprise=0 -f filter-enterprise.awk md/$*.md | sed -e "/\[INDENT\]/ {s/\[INDENT\]//}" | sed -e "/\[EDITION\]/ {s//Open/}" | ronn -h  > md/platform-$*.html

enterprise-man/%: md/%.md
	@echo Building $@
	@$(DEBUG) mkdir -p enterprise-man
	@$(DEBUG) awk -v enterprise=1 -f filter-enterprise.awk md/$*.md | sed -e "/\[EDITION\]/ {s//Enterprise/}" | ronn -r > md/$*
	@$(DEBUG) sed -e "/^.P$$/ {N; s/.P\n\[INDENT\]/.IP\n/}" md/$* > $@
	rm md/$*
# Generate an HTML version of the page
#	@$(DEBUG) awk -v enterprise=1 -f filter-enterprise.awk md/$*.md | sed -e "/\[INDENT\]/ {s/\[INDENT\]//}" | sed -e "/\[EDITION\]/ {s//Enterprise/}" | ronn -h  > md/enterprise-$*.html
	@echo Copy $@ to PBIS-Enterprise/src/linux/docs/man

all: manpages

manpages: platform-manpages enterprise-manpages

platform-manpages: $(patsubst md/%.md,man/%,$(MANPAGES))

enterprise-manpages: $(patsubst md/%.md,enterprise-man/%,$(MANPAGES))

.PHONY: clean
clean:
		rm -f man/*.?
		rm -f enterprise-man/*.?
		rm -f md/*.?

