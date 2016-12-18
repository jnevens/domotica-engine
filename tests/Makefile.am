AM_CPPFLAGS =
AM_CFLAGS = -pthread -lcheck_pic -lrt -lm -lsubunit
LDADD =

TESTS = check
@VALGRIND_CHECK_RULES@

check_PROGRAMS = $(TESTS)

check_SOURCES = check.c
