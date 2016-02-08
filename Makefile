EXTENSION = weighted_stats
EXTVERSION = 0.2.0
DOCS = README
TESTS = $(wildcard test/sql/*.sql)
REGRESS = $(patsubst test/sql/%.sql,%,$(TESTS))
REGRESS_OPTS = --inputdir=test
DOCS         = doc/weighted_stats.md
MODULES = $(patsubst %.c,%, src/weighted_stats.c)
PG_CONFIG = pg_config

all: sql/$(EXTENSION)--$(EXTVERSION).sql

sql/$(EXTENSION)--$(EXTVERSION).sql: sql/$(EXTENSION).sql
	mkdir -p $(@D)
	cp $< $@

# this build extension.control from extension.control.in
$(EXTENSION).control: $(EXTENSION).control.in
	sed 's/EXTVERSION/$(EXTVERSION)/;s/EXTENSION/$(EXTENSION)/;s/EXTCOMMENT/$(EXTCOMMENT)/' $< > $@

release-zip: all
	git archive --format zip --prefix=weighted_stats-$(EXTVERSION)/ --output ./weighted_stats-$(EXTVERSION).zip master

DATA = sql/$(EXTENSION)--$(EXTVERSION).sql
EXTRA_CLEAN = sql/$(EXTENSION)--$(EXTVERSION).sql

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

%/.dir:
	mkdir -p $(@D)
	touch $@

dirsToCreate := $(patsubst %,%.dir,$(dir $(OBJS) $(MODULES)))

modulesObjects := $(patsubst %,%.o,$(MODULES))


$(OBJS) $(modulesObjects): $(dirsToCreate)


deb: $(EXTENSION).control
	make clean
	make -f debian/rules debian/control
	dh clean
	make -f debian/rules orig
	debuild -us -uc -sa
