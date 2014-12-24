AUTOMAKE_OPTIONS	= foreign subdir-objects
ACLOCAL_AMFLAGS		= -I m4
SHELL			= /bin/bash

AM_CFLAGS		= $(FUSE_CFLAGS) $(WFLAGS)

bin_PROGRAMS		= g60cuse
g60cuse_SOURCES		= g60cuse.c
g60cuse_LDADD		= $(LDADD) $(FUSE_LIBS)

dist_noinst_SCRIPTS	= autogen.sh

EXTRA_DIST		= .gitignore
DISTCLEANFILES		= *~
