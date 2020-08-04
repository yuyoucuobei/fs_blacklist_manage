include $(top_srcdir)/build/modmake.rulesam
MODNAME=libmod_as_dbcache


if SYSTEM_APR
AM_LIBAPR_CFLAGS := $(shell apr-1-config --cflags)
AM_LIBAPR_CPPFLAGS := $(shell apr-1-config --cppflags --includes)
AM_LIBAPR_LDFLAGS := $(shell apr-1-config --ldflags)
AM_LIBAPR_LIBS := $(shell apr-1-config \--libs)
AM_LIBAPR_LINKLIBTOOL := $(shell apr-1-config \--link-libtool)
else
AM_LIBAPR_CFLAGS := $(shell ../../../../libs/apr/apr-1-config --cflags)
AM_LIBAPR_CPPFLAGS := $(shell ../../../../libs/apr/apr-1-config --cppflags --includes)
AM_LIBAPR_LDFLAGS := $(shell ../../../../libs/apr/apr-1-config --ldflags)
AM_LIBAPR_LIBS := $(subst $(switch_builddir)/,,$(shell ../../../../libs/apr/apr-1-config \--libs))
endif
if SYSTEM_APRUTIL
AM_LIBAPU_CPPFLAGS := $(shell apu-1-config --includes)
AM_LIBAPU_LDFLAGS := $(shell apu-1-config --ldflags)
AM_LIBAPU_LIBS := $(shell apu-1-config \--libs)
AM_LIBAPU_LINKLIBTOOL := $(shell apu-1-config \--link-libtool)
else
AM_LIBAPU_CPPFLAGS := $(shell ../../../../libs/apr-util/apu-1-config --includes)
AM_LIBAPU_LDFLAGS := $(shell ../../../../libs/apr-util/apu-1-config --ldflags)
AM_LIBAPU_LIBS := $(subst $(switch_builddir)/,,$(shell ../../../../libs/apr-util/apu-1-config \--libs))
endif


mod_LTLIBRARIES = libmod_as_dbcache.la
libmod_as_dbcache_la_SOURCES  = mod_as_dbcache.c mod_as_dbcache_mysql.c mod_as_dbcache_redis.c mod_as_dbcache_queue.c mod_as_dbcache_api.c
libmod_as_dbcache_la_CFLAGS   = -W $(AM_CFLAGS) $(AM_LIBAPR_CFLAGS) $(AM_LIBAPR_CPPFLAGS) -I../mod_as_numchange -I../mod_as_common 
libmod_as_dbcache_la_LIBADD   = $(switch_builddir)/libfreeswitch.la $(AM_LIBAPR_LIBS) /usr/local/lib/libhiredis_vip.a -lmysqlclient_r -L/usr/lib64/mysql 
libmod_as_dbcache_la_LDFLAGS  = -avoid-version -module -no-undefined -shared $(AM_LIBAPR_LDFLAGS)