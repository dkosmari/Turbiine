EXTRA_DIST = \
	bootstrap \
	COPYING \
	docker-build.sh \
	Dockerfile \
	README.md


SUBDIRS = external/libwupsxx


AM_CPPFLAGS = \
	-I$(top_srcdir)/external/libwupsxx/include \
	$(DEVKITPRO_CPPFLAGS)

AM_CXXFLAGS = \
	$(DEVKITPRO_CXXFLAGS) \
	-Wall -Wextra -Werror

AM_LDFLAGS = $(DEVKITPRO_LDFLAGS)

LIBS = $(DEVKITPRO_LIBS)


WPS_FILE = turbiine.wps

noinst_PROGRAMS = turbiine.elf

turbiine_elf_SOURCES = \
	src/cfg.cpp src/cfg.hpp \
	src/main.cpp \
	src/vpad.cpp src/vpad.hpp \
	src/wpad.cpp src/wpad.hpp

turbiine_elf_LDADD = $(top_builddir)/external/libwupsxx/src/libwupsxx.a



all-local: $(WPS_FILE)


install-exec-local: all
	curl  --upload-file $(WPS_FILE) "ftp://wiiu:/fs/vol/external01/wiiu/environments/aroma/plugins/"


uninstall-local:
	curl "ftp://wiiu" --quote "DELE /fs/vol/external01/wiiu/environments/aroma/plugins/$(WPS_FILE)"


.PHONY: run
run: all
	WIILOAD=tcp:wiiu wiiload $(WPS_FILE)



.PHONY: company
company: compile_flags.txt

compile_flags.txt: Makefile
	printf "%s" "$(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS)" | xargs -n1 | sort -u > $(srcdir)/compile_flags.txt
	$(CPP) -xc++ /dev/null -E -Wp,-v 2>&1 | sed -n 's,^ ,-I,p' >> $(srcdir)/compile_flags.txt



@INC_AMINCLUDE@
DISTCLEANFILES = $(AMINCLUDE)

CLEANFILES = $(WPS_FILE)
