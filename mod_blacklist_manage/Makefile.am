include $(top_srcdir)/build/modmake.rulesam
MODNAME=mod_blacklist_manage

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

mod_LTLIBRARIES = mod_blacklist_manage.la
mod_blacklist_manage_la_SOURCES  = mod_blacklist_manage.cpp mod_blacklist_manage_api.cpp mod_blacklist_manage_app.cpp mod_blacklist_manage_data.cpp mod_blacklist_manage_record.cpp mod_blacklist_manage_deduction.cpp mod_blacklist_manage_user.cpp mod_blacklist_manage_mysql.cpp 
mod_blacklist_manage_la_CPPFLAGS   = -W $(AM_CPPFLAGS) $(AM_LIBAPR_CPPFLAGS) -I../mod_as_dbcache/
mod_blacklist_manage_la_LIBADD   = $(switch_builddir)/libfreeswitch.la $(AM_LIBAPR_LIBS) -lmysqlclient_r -L/usr/lib64/mysql 
mod_blacklist_manage_la_LDFLAGS  = -avoid-version -module -no-undefined -shared $(AM_LIBAPR_LDFLAGS) -lmod_as_dbcache -L../mod_as_dbcache/.libs/

