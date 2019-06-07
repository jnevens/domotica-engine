AM_CPPFLAGS = -I$(src_buildir)/include -I$(top_srcdir)/include
LDADD =
domotica_engine_dir = $(sysconfdir)/domotica-engine

sbin_PROGRAMS = domotica-engine domctl

domotica_engine_LDADD = -leu $(LDADD) -lm -lsqlite3 -lcurl -ljson-c
domotica_engine_CFLAGS = -rdynamic -funwind-tables -D_GNU_SOURCE
domotica_engine_SOURCES = \
	action.c \
	arguments.c \
	bool.c \
	condition.c \
	device.c \
	device_list.c \
	dht22.c \
	engine.c \
	event.c \
	gpio.c \
	line_parser.c \
	main.c \
	remote.c \
	rule.c \
	rule_list.c \
	rule_parser.c \
	schedule.c \
	store.c \
	sunriset.c \
	technologies.c \
	timer.c \
	utils_http.c \
	utils_gpio.c \
	utils_sched.c \
	utils_sunriset.c \
	utils_time.c

if HAVE_MOD_ELTAKO
domotica_engine_LDADD += -lvsb -leltako
domotica_engine_SOURCES += \
	eltako.c \
	eltako_fsb14.c \
	eltako_fsr14.c \
	eltako_fts14em.c \
	eltako_fud14.c
endif

if HAVE_MOD_SOLTRACK
domotica_engine_LDADD += -lSolTrack
domotica_engine_SOURCES += \
	soltracker.c
endif

if HAVE_MOD_MUSICCAST
domotica_engine_LDADD += -lcurl -lmusiccast
domotica_engine_SOURCES += \
	yamaha_mc.c
endif

domctl_LDADD = -leu
domctl_CFLAGS =
domctl_SOURCES = \
	domctl.c

install-data-local:
	mkdir -p $(DESTDIR)/$(domotica_engine_dir)/rules.d/

