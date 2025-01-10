#
# Copyright 2010 by Virginia Polytechnic Institute and State
# University. All rights reserved. Virginia Polytechnic Institute and
# State University (Virginia Tech) owns the software and its
# associated documentation.
#

bin_PROGRAMS += bfs
bin_PROGRAMS += createbfs
bin_PROGRAMS += createbfs_rodinia

bfs_SOURCES = graph-traversal/bfs/bfs.cpp
createbfs_SOURCES = graph-traversal/bfs/createbfs.c
createbfs_rodinia_SOURCES = graph-traversal/bfs/graphgen.cpp

#LIBS += -O0 -g

all_local += bfs-all-local
exec_local += bfs-exec-local

bfs-all-local:
	cp $(top_srcdir)/graph-traversal/bfs/bfs_kernel.cl .

bfs-exec-local:
	cp $(top_srcdir)/graph-traversal/bfs/bfs_kernel.cl ${DESTDIR}${bindir}
