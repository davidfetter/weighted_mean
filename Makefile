EXTENSION = weighted_stats
EXTVERSION = 0.1.0
DOCS = README
TESTS = $(wildcard test/sql/*.sql)
REGRESS = $(patsubst test/sql/%.sql,%,$(TESTS))
REGRESS_OPTS = --inputdir=test
DOCS         = doc/weighted_stats.md
MODULES = $(patsubst %.c,%, src/weighted_stats.c)
PG_CONFIG = pg_config
PG91 = $(shell $(PG_CONFIG) --version | grep -qE " 8\.| 9\.0" && echo no || echo yes)

ifeq ($(PG91),yes)
all: sql/$(EXTENSION)--$(EXTVERSION).sql




sql/$(EXTENSION)--$(EXTVERSION).sql: sql/$(EXTENSION).sql
	mkdir -p $(@D)
	cp $< $@

# this build extension.control from extension.control.in
$(EXTENSION).control: $(EXTENSION).control.in
	sed 's/EXTVERSION/$(EXTVERSION)/;s/EXTENSION/$(EXTENSION)/;s/EXTCOMMENT/$(EXTCOMMENT)/' $< > $@

release-zip: all
	git archive --format zip --prefix=weighted_stats-$(EXTVERSION)/ --output ./weighted_stats-$(EXTVERSION).zip master

DATA = $(wildcard sql/*--*.sql) sql/$(EXTENSION)--$(EXTVERSION).sql
EXTRA_CLEAN = sql/$(EXTENSION)--$(EXTVERSION).sql
endif

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
