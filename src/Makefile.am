AM_CPPFLAGS =
LDADD =

sbin_PROGRAMS = domotica-engine

domotica_engine_LDADD = -lvsb -lbus -leltako $(LDADD)
domotica_engine_SOURCES = \
	action.c \
	arguments.c \
	eltako.c \
	eltako_fts14em.c \
	eltako_fud14.c \
	engine.c \
	event.c \
	gpio.c \
	input.c \
	input_list.c \
	main.c \
	output.c \
	output_list.c \
	rule.c \
	rule_list.c \
	rule_parser.c \
	technologies.c \
	utils_time.c
