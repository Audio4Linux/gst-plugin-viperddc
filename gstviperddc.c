#include "gstviperddc.h"

/**
 * SECTION:element-viperddc
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch audiotestsrc ! audioconverter ! viperddc ! ! audioconverter ! autoaudiosink
 * ]|
 * </refsect2>
 */

#include <stdio.h>
#include <string.h>
#include <gst/gst.h>
#include <gst/base/base.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>
#include <gst/controller/controller.h>
#include <math.h>
#include <locale.h>

GST_DEBUG_CATEGORY_STATIC (gst_viperddc_debug);
#define GST_CAT_DEFAULT gst_viperddc_debug
/* Filter signals and args */
enum {
    /* FILL ME */
    LAST_SIGNAL
};

enum {
    PROP_0,

    PROP_DDC_ENABLE,
    PROP_DDC_FILE,
};

#define gst_viperddc_parent_class parent_class

G_DEFINE_TYPE (Gstviperddc, gst_viperddc, GST_TYPE_AUDIO_FILTER);

static void gst_viperddc_set_property(GObject *object, guint prop_id,
                                      const GValue *value, GParamSpec *pspec);

static void gst_viperddc_get_property(GObject *object, guint prop_id,
                                      GValue *value, GParamSpec *pspec);

static void gst_viperddc_finalize(GObject *object);

static gboolean gst_viperddc_setup(GstAudioFilter *self,
                                   const GstAudioInfo *info);

static gboolean gst_viperddc_stop(GstBaseTransform *base);

static GstFlowReturn gst_viperddc_transform_ip(GstBaseTransform *base,
                                               GstBuffer *outbuf);

/* GObject vmethod implementations */

/* initialize the viperddc's class */
static void
gst_viperddc_class_init(GstviperddcClass *klass) {

    GObjectClass *gobject_class = (GObjectClass *) klass;
    GstElementClass *gstelement_class = (GstElementClass *) klass;
    GstBaseTransformClass *basetransform_class = (GstBaseTransformClass *) klass;
    GstAudioFilterClass *audioself_class = (GstAudioFilterClass *) klass;
    GstCaps *caps;

    /* debug category for fltering log messages
     */
    GST_DEBUG_CATEGORY_INIT (gst_viperddc_debug, "viperddc", 0, "viperddc element");

    gobject_class->set_property = gst_viperddc_set_property;
    gobject_class->get_property = gst_viperddc_get_property;
    gobject_class->finalize = gst_viperddc_finalize;

    /* global switch */
    g_object_class_install_property(gobject_class, PROP_DDC_ENABLE,
                                    g_param_spec_boolean("ddc_enable", "DDCEnabled", "Enable processing",
                                                         FALSE, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

    g_object_class_install_property(gobject_class, PROP_DDC_FILE,
                                    g_param_spec_string("ddc_file", "DDCFilePath", "VDC path",
                                                        "", G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

    gst_element_class_set_static_metadata(gstelement_class,
                                          "viperddc",
                                          "Filter/Effect/Audio",
                                          "ViPER-DDC open-source replacement for GStreamer1",
                                          "ThePBone <tim.schneeberger@outlook.de>");

    caps = gst_caps_from_string(ALLOWED_CAPS);
    gst_audio_filter_class_add_pad_templates(GST_VIPERDDC_CLASS (klass), caps);
    gst_caps_unref(caps);

    audioself_class->setup = GST_DEBUG_FUNCPTR (gst_viperddc_setup);
    basetransform_class->transform_ip =
            GST_DEBUG_FUNCPTR (gst_viperddc_transform_ip);
    basetransform_class->transform_ip_on_passthrough = FALSE;
    basetransform_class->stop = GST_DEBUG_FUNCPTR (gst_viperddc_stop);
}

/* initialize the new element
 * allocate private resources
 */
static void
gst_viperddc_init(Gstviperddc *self) {
    gst_base_transform_set_in_place(GST_BASE_TRANSFORM (self), TRUE);
    gst_base_transform_set_gap_aware(GST_BASE_TRANSFORM (self), TRUE);

    /* initialize properties */
    self->ddc_enable = FALSE;
    self->ddc_file = malloc(256);
    memset(self->ddc_file, 0, 256);

    g_mutex_init(&self->lock);
}

char *memory_read_ascii(char *path) {
    int c;
    long size;
    FILE *file;
    int i = 0;
    file = fopen(path, "r");
    if (file) {
        fseek(file, 0, SEEK_END);
        size = ftell(file);
        fseek(file, 0, SEEK_SET);
        char *buffer = (char *) malloc(size * sizeof(char));
        while ((c = getc(file)) != EOF) {
            buffer[i] = (char) c;
            i++;
        }
        fclose(file);
        return buffer;
    }
    return NULL;
}

static void sync_parameters(GObject *object) {
    Gstviperddc *self = GST_VIPERDDC (object);

    setlocale(LC_NUMERIC, "C");
    if (!self->ddc_enable) {
        if (self->sosCount) {
            for (int i = 0; i < self->sosCount; i++) {
                free(self->df441[i]);
                free(self->df48[i]);
            }
            free(self->df441);
            self->df441 = 0;
            free(self->df48);
            self->df48 = 0;
            self->sosCount = 0;
            self->sosPointer = 0;
        }
        return;
    }

    if (!self->ddc_file || self->ddc_file == NULL) {
        GST_CAT_ERROR(gst_viperddc_debug, "DDC path is not set");
        return;
    }
    char *ddcString = memory_read_ascii(self->ddc_file);
    if (!ddcString || ddcString == NULL) {
        GST_CAT_ERROR(gst_viperddc_debug, "Unable to open DDC file");
        return;
    }
    int d = 0;
    for (int i = 0; i < strlen(ddcString); ++i) {
        d |= ddcString[i];
    }
    if (d == 0) {
        GST_CAT_ERROR(gst_viperddc_debug, "DDC contents contain no data");
        if (ddcString != NULL) {
            free(ddcString);
        }
        return;
    }
    int begin = strcspn(ddcString, "S");
    if (strcspn(ddcString, "R") != begin + 1) { //check for 'SR' in the string
        GST_CAT_ERROR(gst_viperddc_debug, "Invalid DDC string");
        if (ddcString != NULL) {
            free(ddcString);
        }
        return;
    }

    self->sosCount = DDCParser(ddcString, &self->df441, &self->df48);

    GST_CAT_DEBUG(gst_viperddc_debug, "SOS count %d", self->sosCount);

    if(self->sosCount < 1){
        GST_CAT_ERROR(gst_viperddc_debug, "SOS count is zero");
    }

    if (self->samplerate == 44100 && self->df441) {
        self->sosPointer = self->df441;
        self->usedSOSCount = self->sosCount;
    } else if (self->samplerate == 48000 && self->df48) {
        self->sosPointer = self->df48;
        self->usedSOSCount = self->sosCount;
    } else {
        GST_CAT_ERROR(gst_viperddc_debug, "Invalid sampling rate");
    }

    if (ddcString != NULL) {
        free(ddcString);
    }

    GST_CAT_DEBUG(gst_viperddc_debug, "VDC num of SOS: %d", self->sosCount);
    GST_CAT_DEBUG(gst_viperddc_debug, "VDC df48[0].b0: %1.14f", (float) self->df48[0]->b0);
}

/* free private resources
*/
static void
gst_viperddc_finalize(GObject *object) {
    Gstviperddc *self = GST_VIPERDDC (object);

    self->ddc_enable = FALSE;
    sync_parameters(object);

    g_mutex_clear(&self->lock);

    G_OBJECT_CLASS (parent_class)->finalize(object);
}

static void
gst_viperddc_set_property(GObject *object, guint prop_id,
                          const GValue *value, GParamSpec *pspec) {
    Gstviperddc *self = GST_VIPERDDC (object);

    switch (prop_id) {
        case PROP_DDC_ENABLE: {
            g_mutex_lock(&self->lock);
            self->ddc_enable = g_value_get_boolean(value);
            sync_parameters(object);
            g_mutex_unlock(&self->lock);
        }
            break;

        case PROP_DDC_FILE: {
            g_mutex_lock(&self->lock);
            if (strlen(g_value_get_string(value)) < 256) {
                self->ddc_file = malloc(256);
                strncpy(self->ddc_file,
                        g_value_get_string(value), 256);
                sync_parameters(object);
            }
            g_mutex_unlock(&self->lock);
        }
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gst_viperddc_get_property(GObject *object, guint prop_id,
                          GValue *value, GParamSpec *pspec) {
    Gstviperddc *self = GST_VIPERDDC (object);

    switch (prop_id) {
        case PROP_DDC_ENABLE:
            g_value_set_boolean(value, self->ddc_enable);
            break;

        case PROP_DDC_FILE:
            g_value_set_string(value, self->ddc_file);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }

}

/* GstBaseTransform vmethod implementations */

static gboolean
gst_viperddc_setup(GstAudioFilter *base, const GstAudioInfo *info) {
    Gstviperddc *self = GST_VIPERDDC (base);
    if (info) {
        self->samplerate = GST_AUDIO_INFO_RATE (info);
    } else {
        self->samplerate = GST_AUDIO_FILTER_RATE (self);
    }
    if (self->samplerate <= 0)
        return FALSE;

    GST_DEBUG_OBJECT (self, "current sample_rate = %d", self->samplerate);

    sync_parameters((GObject *) self);

    return TRUE;
}

static gboolean
gst_viperddc_stop(GstBaseTransform *base) {
    Gstviperddc *self = GST_VIPERDDC (base);
    return TRUE;
}

/* this function does the actual processing
 */
static GstFlowReturn
gst_viperddc_transform_ip(GstBaseTransform *base, GstBuffer *buf) {
    Gstviperddc *filter = GST_VIPERDDC (base);
    guint idx, num_samples;
    float *pcm_data;
    GstClockTime timestamp, stream_time;
    GstMapInfo map;

    timestamp = GST_BUFFER_TIMESTAMP (buf);
    stream_time =
            gst_segment_to_stream_time(&base->segment, GST_FORMAT_TIME, timestamp);

    if (GST_CLOCK_TIME_IS_VALID (stream_time))
        gst_object_sync_values(GST_OBJECT (filter), stream_time);

    if (G_UNLIKELY (GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_GAP)))
        return GST_FLOW_OK;

    if (filter->ddc_enable) {
        gst_buffer_map(buf, &map, GST_MAP_READWRITE);
        num_samples = map.size / GST_AUDIO_FILTER_BPS (filter) / 2;
        pcm_data = (float *) (map.data);

        g_mutex_lock(&filter->lock);

        int framePos, framePos2x;
        for (framePos = 0; framePos < num_samples; framePos++) {
            framePos2x = framePos << 1;

            int indexL = framePos2x;
            int indexR = framePos2x + 1;

            for (int j = 0; j < filter->usedSOSCount; j++) {
                SOS_DF2_Float_StereoProcess(filter->sosPointer[j], pcm_data[indexL], pcm_data[indexR],
                                            &pcm_data[indexL], &pcm_data[indexR]);
            }
        }

        g_mutex_unlock(&filter->lock);
        gst_buffer_unmap(buf, &map);
    }

    return GST_FLOW_OK;
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
viperddc_init(GstPlugin *viperddc) {
    return gst_element_register(viperddc, "viperddc", GST_RANK_NONE,
                                GST_TYPE_VIPERDDC);
}

/* gstreamer looks for this structure to register viperddcs
 */
GST_PLUGIN_DEFINE (
        GST_VERSION_MAJOR,
        GST_VERSION_MINOR,
        viperddc,
        "ViperDDC element",
        viperddc_init,
        VERSION,
        "GPL",
        "GStreamer",
        "http://gstreamer.net/"
)
