AM_CPPFLAGS = -I$(src_buildir)/include -I$(top_srcdir)/include
LDADD =

sbin_PROGRAMS = domotica-engine

domotica_engine_LDADD = -lvsb -lbus -leltako $(LDADD) -lm
domotica_engine_SOURCES = \
	action.c \
	arguments.c \
	device.c \
	device_list.c \
	dht22.c \
	eltako.c \
	eltako_fsb14.c \
	eltako_fts14em.c \
	eltako_fud14.c \
	engine.c \
	event.c \
	gpio.c \
	main.c \
	rule.c \
	rule_list.c \
	rule_parser.c \
	sunriset.c \
	technologies.c \
	timer.c \
	utils_gpio.c \
	utils_sched.c \
	utils_sunriset.c \
	utils_time.c
