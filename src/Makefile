TARGETS = subdirs

SUBDIRS = \
  util \
  kernel \
  gamecode \
  triggers \
  items \
  attributes \
  races \
  spells \
  modules \
  eressea \
  mapper

include Makefile.include


## subdirectories

publish-debug publish-release publish-profile clean depend::
	@mkdir -p $(PUBLISH_DIR)
	@for subdir in $(SUBDIRS); do \
	if [ -d $$subdir ]; then \
	echo --\> Making $@ in $$subdir ; \
	mkdir -p $$subdir/$(BUILD_DIR) ; \
	$(MAKE) -C $$subdir -$(MAKEFLAGS) $@ || exit 1; \
	fi \
	done

## more definitions
