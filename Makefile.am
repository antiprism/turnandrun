ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src

EXTRA_DIST = README.md NEWS AUTHORS COPYING \
	bootstrap commands.txt \
	\
	scripts

docdir = @docdir@
doc_DATA = README.md NEWS AUTHORS COPYING

bin_SCRIPTS = /
	scripts/turnandrun_launch_test \
	scripts/turnandrun_service_install \
	scripts/turnandrun_service_uninstall

format_all:
	for f in src/*.cpp src/*.h ; do \
	clang-format -style=file -i $$f; \
	done

