AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CFLAGS = -pthread -lcheck_pic -lrt -lm -lsubunit
LDADD =

TESTS = check_test

@VALGRIND_CHECK_RULES@

check_PROGRAMS = $(TESTS)

check_test_SOURCES = check_test.c
