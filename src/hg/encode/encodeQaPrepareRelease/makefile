# Build encode loader

include ../../../inc/common.mk

SCRIPT_FILES = \
	encodeQaPrepareRelease	

SCRIPT_INSTALL = "/cluster/bin/scripts"

all:
	@${MAKE} --no-print-directory SCRIPTS=${HOME}/bin/scripts scripts

alpha:
	@${MAKE} --no-print-directory SCRIPTS=${SCRIPT_INSTALL} scripts

install: alpha

scripts: ${SCRIPT_FILES:%=${SCRIPTS}/%}

${SCRIPTS}/%: %
	@mkdir -p $(dir $@)
	chmod +x $<
	cp -p $< $@.tmp
	mv -f $@.tmp $@

clean::
	echo nothing to clean in hg/encode/encodeQaPrepareRelease/
