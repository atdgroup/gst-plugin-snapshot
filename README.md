# gst-plugin-snapshot
GStreamer 1.0 element that can save a single frame as a snapshot image from a video stream.

Properties:
  trigger             : If true the next frame after the frame delay, is written to an image file.
  framedelay          : The delay in frames from the trigger to the image save.
  filetype            : Specify the file type ("bmp", "png" or "jpeg").
  location            : Specify the file path and filename to write to.

----------
First run autogen.sh
$ chmod a+x autogen.sh
$ ./autogen.sh
This will use autotools to setup the dev environment, and complete with the line:
"Now type 'make' to compile this module."
You can do that.
$ make

$ sudo make install 
will put install the lo file for use with GStreamer, in /usr/local/lib/gstreamer-1.0
To use this in a pipeline you need to tell gstreamer where to find the .lo file.
use:
$ export GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0


pipelines
---------
Working:
gst-launch-1.0 videotestsrc num-buffers=20 ! snapshotfilter trigger=true framedelay=15 filetype="jpeg" location="image.jpg" ! videoconvert ! xvimagesink

Gstreamer plugin locations:
/usr/lib/i386-linux-gnu/gstreamer-1.0
/usr/local/lib/gstreamer-1.0


WHAT IT IS
----------

gst-plugin is a template for writing your own GStreamer plug-in.

The code is deliberately kept simple so that you quickly understand the basics
of how to set up autotools and your source tree.

This template demonstrates :
- what to do in autogen.sh
- how to setup configure.ac (your package name and version, GStreamer flags)
- how to setup your source dir 
- what to put in Makefile.am

More features and templates might get added later on.

HOW TO USE IT
-------------

To use it, either make a copy for yourself and rename the parts or use the
make_element script in tools. To create sources for "myfilter" based on the
"gsttransform" template run:

cd src;
../tools/make_element myfilter gsttransform

This will create gstmyfilter.c and gstmyfilter.h. Open them in an editor and
start editing. There are several occurances of the string "template", update
those with real values. The plugin will be called 'myfilter' and it will have
one element called 'myfilter' too. Also look for "FIXME:" markers that point you
to places where you need to edit the code.

You still need to adjust the Makefile.am.

