/* gstnvwrapdec.c - nvv4l2decoder ! nvvidconv ! video/x-raw as one decoder element.
 * MIT. Build: see build.sh
 */

#include <gst/gst.h>

#define PACKAGE "gst-nvwrapdec"
#define VERSION "1.0"

GST_DEBUG_CATEGORY_STATIC (nvwrapdec_debug);
#define GST_CAT_DEFAULT nvwrapdec_debug

typedef struct _GstNvWrapDec
{
  GstBin parent;
} GstNvWrapDec;

typedef struct _GstNvWrapDecClass
{
  GstBinClass parent_class;
} GstNvWrapDecClass;

#define GST_TYPE_NVWRAPDEC (gst_nvwrapdec_get_type ())
G_DEFINE_TYPE (GstNvWrapDec, gst_nvwrapdec, GST_TYPE_BIN);

/* input side mirrors nvv4l2decoder's sink template (r39.2.1) */
static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK, GST_PAD_ALWAYS,
    GST_STATIC_CAPS (
        "video/x-h264, stream-format=(string)byte-stream, alignment=(string)au; "
        "video/x-h265, stream-format=(string)byte-stream, alignment=(string)au; "
        "video/x-vp8; "
        "video/x-vp9; "
        "video/x-av1, stream-format=(string)obu-stream, alignment=(string)frame"));

/* plain system memory out, that's the whole point */
static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC, GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw, format=(string){ NV12, I420 }"));

static void
gst_nvwrapdec_class_init (GstNvWrapDecClass * klass)
{
  GstElementClass *eclass = GST_ELEMENT_CLASS (klass);

  gst_element_class_add_static_pad_template (eclass, &sink_template);
  gst_element_class_add_static_pad_template (eclass, &src_template);

  gst_element_class_set_static_metadata (eclass,
      "NVDEC wrapper video decoder",
      "Codec/Decoder/Video",
      "nvv4l2decoder + nvvidconv in one bin so decodebin/playbin can use NVDEC",
      "xadk");
}

static void
gst_nvwrapdec_init (GstNvWrapDec * self)
{
  GstElement *dec, *conv, *filt;
  GstCaps *caps;
  GstPad *pad;

  dec = gst_element_factory_make ("nvv4l2decoder", "dec");
  conv = gst_element_factory_make ("nvvidconv", "conv");
  filt = gst_element_factory_make ("capsfilter", "filt");

  if (!dec || !conv || !filt) {
    GST_ERROR_OBJECT (self, "missing nv elements (not an L4T system?)");
    g_clear_object (&dec);
    g_clear_object (&conv);
    g_clear_object (&filt);
    return;
  }

  /* force raw output, no NVMM */
  caps = gst_caps_from_string ("video/x-raw, format=(string){ NV12, I420 }");
  g_object_set (filt, "caps", caps, NULL);
  gst_caps_unref (caps);

  gst_bin_add_many (GST_BIN (self), dec, conv, filt, NULL);

  if (!gst_element_link_many (dec, conv, filt, NULL)) {
    GST_ERROR_OBJECT (self, "failed to link dec ! conv ! filt");
    return;
  }

  pad = gst_element_get_static_pad (dec, "sink");
  gst_element_add_pad (GST_ELEMENT (self), gst_ghost_pad_new ("sink", pad));
  gst_object_unref (pad);

  pad = gst_element_get_static_pad (filt, "src");
  gst_element_add_pad (GST_ELEMENT (self), gst_ghost_pad_new ("src", pad));
  gst_object_unref (pad);
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (nvwrapdec_debug, "nvwrapdec", 0,
      "NVDEC wrapper decoder");

  /* nvv4l2decoder is PRIMARY+11, go one above so autopluggers try us first */
  return gst_element_register (plugin, "nvwrapdec",
      GST_RANK_PRIMARY + 12, GST_TYPE_NVWRAPDEC);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR,
    nvwrapdec,
    "NVDEC wrapper decoder for Jetson",
    plugin_init, VERSION, "MIT/X11", PACKAGE,
    "https://github.com/xadk/jetson-gst-nvwrapdec/")
