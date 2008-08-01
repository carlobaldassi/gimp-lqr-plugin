## Process this file with automake to produce Makefile.in

bin_PROGRAMS = plug_in_lqr_iter

bindir = $(GIMP_LIBDIR)/plug-ins

plug_in_lqr_iter_SOURCES = \
	plug_in_lqr_iter.c \
	../src/main_common.h

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(LOCALEDIR)"\"		\
	-DDATADIR=\""$(DATADIR)"\"\
	-I$(top_srcdir)		\
	@GIMP_CFLAGS@		\
	-I$(includedir)

LDADD = $(GIMP_LIBS)

