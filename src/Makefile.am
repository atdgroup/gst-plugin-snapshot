# Note: plugindir is set in configure

plugin_LTLIBRARIES = libsnapshotfilter.la

# Path to installation of the output SDK 
#SNAPSHOT_CFLAGS = 
#SNAPSHOT_LIBS = 

GTK_CFLAGS = `pkg-config --cflags gtk+-3.0`
GTK_LIBS = `pkg-config --libs gtk+-3.0`

# sources used to compile this plug-in
libsnapshotfilter_la_SOURCES = gstsnapshotfilter.c gstsnapshotfilter.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libsnapshotfilter_la_CFLAGS = $(GST_CFLAGS) $(GTK_CFLAGS)
libsnapshotfilter_la_LIBADD = $(GST_LIBS) -lgstvideo-1.0 $(SNAPSHOT_LIBS)
libsnapshotfilter_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) $(GTK_LIBS) -rpath /usr/local/lib
libsnapshotfilter_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstsnapshotfilter.h
