bin_PROGRAMS += stringmatch
bin_PROGRAMS += stringmatch_gen

stringmatch_SOURCES = mapreduce/stringmatch/stringmatch.c
stringmatch_CFLAGS = -g3 -O0

stringmatch_gen_SOURCES = mapreduce/stringmatch/stringmatch_gen.c

all_local += stringmatch-all-local
exec_local += stringmatch-exec-local

stringmatch-all-local:
	cp $(top_srcdir)/mapreduce/stringmatch/stringmatch_kernel.cl .

stringmatch-exec-local:
	cp $(top_srcdir)/mapreduce/stringmatch/stringmatch_kernel.cl ${DESTDIR}${bindir}