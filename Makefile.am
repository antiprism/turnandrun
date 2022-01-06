ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src

EXTRA_DIST = \
	README.md turnandrun_dial.png wiring_ads1x15.png \
	NEWS AUTHORS COPYING bootstrap \
	\
	scripts/turnandrun_launch_test \
	scripts/turnandrun_service_uninstall \
	doc

docdir = @docdir@
doc_DATA = README.md NEWS AUTHORS COPYING doc/FAQ.md \
	   turnandrun_dial.png wiring_ads1x15.png

docresourcesdir = ${docdir}/resources
docresources_DATA = \
        doc/resources/vols_conf.py


bin_SCRIPTS = \
	scripts/turnandrun_launch_test \
	scripts/turnandrun_service_install \
	scripts/turnandrun_service_uninstall

format_all:
	for f in src/*.cpp src/*.h ; do \
	clang-format -style=file -i $$f; \
	done

