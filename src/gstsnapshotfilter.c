/*
 * Snapshot Filter GStreamer Plugin
 * Copyright (C) 2017 Gray Cancer Institute
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-snapshotfilter
 *
 * This element makes some snapshotment on the image, e.g. average or max R, G and B values.
 * With a definable region of interest (ROI).
 * The results will be output to stdout, UART or SPI
 * either in ascii or binary.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m videotestsrc ! snapshotfilter ! fakesink
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gdk/gdk.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "gstsnapshotfilter.h"

GST_DEBUG_CATEGORY_STATIC (gst_snapshotfilter_debug);
#define GST_CAT_DEFAULT gst_snapshotfilter_debug

/* Filter signals and args */
enum
{
	/* FILL ME */
	LAST_SIGNAL
};

enum
{
	PROP_0,
	PROP_TRIGGER,
	PROP_FRAME_DELAY,
	PROP_FILETYPE,
	PROP_FILEPATH
};

#define DEFAULT_PROP_FILETYPE "bmp"
#define DEFAULT_PROP_FILEPATH "image.bmp"

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
		GST_PAD_SINK,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE
				("{ RGB }"))
);

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
		GST_PAD_SRC,
		GST_PAD_ALWAYS,
		GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE
				("{ RGB }"))
);

#define gst_snapshotfilter_parent_class parent_class
G_DEFINE_TYPE (Gstsnapshotfilter, gst_snapshotfilter, GST_TYPE_ELEMENT);

static void gst_snapshotfilter_set_property (GObject * object, guint prop_id,
		const GValue * value, GParamSpec * pspec);
static void gst_snapshotfilter_get_property (GObject * object, guint prop_id,
		GValue * value, GParamSpec * pspec);

static gboolean gst_snapshotfilter_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_snapshotfilter_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);
static void gst_snapshotfilter_finalize (GObject * object);

/* GObject vmethod implementations */

/* initialize the snapshotfilter's class */
static void
gst_snapshotfilter_class_init (GstsnapshotfilterClass * klass)
{
	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;

	gobject_class = (GObjectClass *) klass;
	gstelement_class = (GstElementClass *) klass;

	gobject_class->set_property = gst_snapshotfilter_set_property;
	gobject_class->get_property = gst_snapshotfilter_get_property;
	gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_snapshotfilter_finalize);

	// trigger property
	g_object_class_install_property (gobject_class, PROP_TRIGGER,
			g_param_spec_boolean ("trigger", "Trigger", "If true the next frame after the frame delay, is written to an image file.",
					FALSE, G_PARAM_WRITABLE));
	// frame delay property
	g_object_class_install_property (gobject_class, PROP_FRAME_DELAY,
			g_param_spec_int("framedelay", "Frame delay", "The delay in frames from the trigger to the image save.", 0, INT_MAX, 0,
					(GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
	// Output type property
	g_object_class_install_property (gobject_class, PROP_FILETYPE,
			g_param_spec_string("filetype", "Image file type", "Specify the file type (\"bmp\", \"png\" or \"jpeg\").", DEFAULT_PROP_FILETYPE,
					(GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
	// Output path property
	g_object_class_install_property (gobject_class, PROP_FILEPATH,
			g_param_spec_string("location", "Image file path", "Specify the file path and filename to write to.", DEFAULT_PROP_FILEPATH,
					(GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));

	gst_element_class_set_details_simple(gstelement_class,
			"snapshotfilter",
			"Filter",
			"When asked, take an image from the video stream and save it to an image file.",
			"Paul R Barber <<paul.barber@oncology.ox.ac.uk>>");

	gst_element_class_add_pad_template (gstelement_class,
			gst_static_pad_template_get (&src_factory));
	gst_element_class_add_pad_template (gstelement_class,
			gst_static_pad_template_get (&sink_factory));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_snapshotfilter_init (Gstsnapshotfilter * filter)
{
	filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
	gst_pad_set_event_function (filter->sinkpad,
			GST_DEBUG_FUNCPTR(gst_snapshotfilter_sink_event));
	gst_pad_set_chain_function (filter->sinkpad,
			GST_DEBUG_FUNCPTR(gst_snapshotfilter_chain));
	GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
	gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

	filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
	GST_PAD_SET_PROXY_CAPS (filter->srcpad);
	gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

	filter->trigger = FALSE;
	filter->frame_delay = 0;
	filter->delay_counter = 0;
	strcpy(filter->filetype, DEFAULT_PROP_FILETYPE);
	strcpy(filter->filepath, DEFAULT_PROP_FILEPATH);

	filter->count=0;

	filter->format_is_RGB = FALSE;
}

static void
gst_snapshotfilter_finalize (GObject * object)
{
	Gstsnapshotfilter *filter = GST_SNAPSHOTFILTER (object);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_snapshotfilter_set_property (GObject * object, guint prop_id,
		const GValue * value, GParamSpec * pspec)
{
	Gstsnapshotfilter *filter = GST_SNAPSHOTFILTER (object);

	switch (prop_id) {
	case PROP_TRIGGER:
		filter->trigger = g_value_get_boolean (value);
		break;
	case PROP_FRAME_DELAY:
		filter->frame_delay = g_value_get_int (value);
		filter->delay_counter = filter->frame_delay;
		break;
	case PROP_FILETYPE:
		strcpy(filter->filetype, g_value_get_string(value));
		break;
	case PROP_FILEPATH:
		strcpy(filter->filepath, g_value_get_string(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gst_snapshotfilter_get_property (GObject * object, guint prop_id,
		GValue * value, GParamSpec * pspec)
{
	Gstsnapshotfilter *filter = GST_SNAPSHOTFILTER (object);

	switch (prop_id) {
	case PROP_FRAME_DELAY:
		g_value_set_int (value, filter->frame_delay);
		break;
	case PROP_FILETYPE:
		g_value_set_string (value, filter->filetype);
		break;
	case PROP_FILEPATH:
		g_value_set_string (value, filter->filepath);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean
gst_snapshotfilter_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
	gboolean ret;
	Gstsnapshotfilter *filter;
	gchar *format;

	filter = GST_SNAPSHOTFILTER (parent);

	switch (GST_EVENT_TYPE (event)) {
	case GST_EVENT_CAPS:
	{
		GstCaps *caps;
		GstStructure *structure;

		gst_event_parse_caps (event, &caps);
		/* do something with the caps */

		if(gst_caps_is_fixed (caps)){

			structure = gst_caps_get_structure (caps, 0);

			if (!gst_structure_get_int (structure, "width", &(filter->width)) ||
					!gst_structure_get_int (structure, "height", &(filter->height))) {
				GST_ERROR_OBJECT (filter, "No width/height available\n");
			}


			format = gst_structure_get_string (structure, "format");
			if (!format) {
				GST_ERROR_OBJECT (filter, "No format available\n");
			}

			if (strcmp(format, "RGB")==0){
				filter->format_is_RGB = TRUE;
				GST_DEBUG_OBJECT (filter, "Format is RGB");
			}

			filter->stride = filter->width * 3;  // TODO Not sure how to get this properly, but we have BGR or RGB data

			GST_DEBUG_OBJECT (filter, "The video size of this set of capabilities is %dx%d, %d\n",
					filter->width, filter->height, filter->stride);

			// TODO open the uart/spi interface here rather than use count==0 check below
		}
		else {
			GST_ERROR_OBJECT (filter, "Caps not fixed.\n");
		}

		/* and forward */
		ret = gst_pad_event_default (pad, parent, event);
		break;
	}
	case GST_EVENT_EOS:

		/* and forward */
		ret = gst_pad_event_default (pad, parent, event);
		break;
		default:
			ret = gst_pad_event_default (pad, parent, event);
			break;
	}
	return ret;
}

static void
gst_snapshot_image(Gstsnapshotfilter *filter, GstBuffer *buf)
{
	GstMapInfo minfo;
	GdkPixbuf *pixbuf;
	GError *error=NULL;

	// Access the buffer - READ ONLY IF POSSIBLE
	gst_buffer_map (buf, &minfo, GST_MAP_READ);

	guint8 *img_ptr = minfo.data;

	// Save frame
	pixbuf = gdk_pixbuf_new_from_data(img_ptr, GDK_COLORSPACE_RGB, FALSE, 8, filter->width, filter->height, filter->stride, NULL, NULL);
	if(pixbuf==NULL){
		GST_ERROR_OBJECT (filter, "gdk_pixbuf_new_from_data error: pixbuf==NULL\n");
	}
	else{
		gdk_pixbuf_save(pixbuf, filter->filepath, filter->filetype, &error, NULL);
		g_object_unref(pixbuf);

		if(error!=NULL){
			GST_ERROR_OBJECT (filter, "gdk_pixbuf_save error: %s\n", error->message);
			g_error_free(error);
		}
	}
	gst_buffer_unmap (buf, &minfo);
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_snapshotfilter_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
	Gstsnapshotfilter *filter;

	filter = GST_SNAPSHOTFILTER (parent);

	// Snapshot?
	// When trigger is high, decrement the delay counter
	if(filter->trigger){
		filter->delay_counter--;
	}

	// When delay counter < 0, reset trigger and get snapshot
	if(filter->delay_counter < 0){
		filter->trigger = FALSE;  // reset the trigger
		filter->delay_counter = filter->frame_delay;
		gst_snapshot_image(filter, buf);
	}

	filter->count++;

	// just push out the incoming buffer without touching it
	return gst_pad_push (filter->srcpad, buf);
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
snapshotfilter_init (GstPlugin * snapshotfilter)
{
	/* debug category for fltering log messages
	 *
	 * exchange the string 'Template snapshotfilter' with your description
	 */
	GST_DEBUG_CATEGORY_INIT (gst_snapshotfilter_debug, "snapshotfilter",
			1, "Template snapshotfilter");

	return gst_element_register (snapshotfilter, "snapshotfilter", GST_RANK_NONE,
			GST_TYPE_SNAPSHOTFILTER);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "snapshotfilter"
#endif

/* gstreamer looks for this structure to register snapshotfilters
 *
 * exchange the string 'Template snapshotfilter' with your snapshotfilter description
 */
GST_PLUGIN_DEFINE (
		GST_VERSION_MAJOR,
		GST_VERSION_MINOR,
		snapshotfilter,
		"This filter will save a frame to an image file, when the trigger is set.",
		snapshotfilter_init,
		VERSION,
		"LGPL",
		PACKAGE_NAME,
		"http://users.ox.ac.uk/~atdgroup"
)
