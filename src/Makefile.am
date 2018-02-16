# Note: plugindir is set in configure

plugin_LTLIBRARIES = libsnapshotplugin.la

# Path to installation of the output SDK 
#SNAPSHOT_CFLAGS = 
#SNAPSHOT_LIBS = 

GTK_CFLAGS = `pkg-config --cflags gtk+-3.0`
GTK_LIBS = `pkg-config --libs gtk+-3.0`

# sources used to compile this plug-in
libsnapshotplugin_la_SOURCES = gstsnapshotfilter.c gstsnapshotfilter.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libsnapshotplugin_la_CFLAGS = $(GST_CFLAGS) $(GTK_CFLAGS)
libsnapshotplugin_la_LIBADD = $(GST_LIBS) -lgstvideo-1.0 $(SNAPSHOT_LIBS)
libsnapshotplugin_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) $(GTK_LIBS) -rpath /usr/local/lib
libsnapshotplugin_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstsnapshotfilter.h
