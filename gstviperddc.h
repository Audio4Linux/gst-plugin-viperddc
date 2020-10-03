#ifndef GST_PLUGIN_VIPERDDC_GSTVIPERDDC_H
#define GST_PLUGIN_VIPERDDC_GSTVIPERDDC_H

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>
#include "vdc.h"

G_BEGIN_DECLS

#define GST_TYPE_VIPERDDC           (gst_viperddc_get_type())
#define GST_VIPERDDC(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VIPERDDC,Gstviperddc))
#define GST_VIPERDDC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GST_TYPE_VIPERDDC,GstviperddcClass))
#define GST_VIPERDDC_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GST_TYPE_VIPERDDC,GstviperddcClass))
#define GST_IS_VIPERDDC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VIPERDDC))
#define GST_IS_VIPERDDC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GST_TYPE_VIPERDDC))

#define PACKAGE "viperddc-plugin"
#define VERSION "1.0.0"

#define ALLOWED_CAPS \
  "audio/x-raw,"                            \
  " format=(string){"GST_AUDIO_NE(F32)"},"  \
  " rate=(int)[44100,48000],"                 \
  " channels=(int)2,"                       \
  " layout=(string)interleaved"

typedef struct _Gstviperddc     Gstviperddc;
typedef struct _GstviperddcClass GstviperddcClass;

struct _Gstviperddc {
    GstAudioFilter audiofilter;

    /* properties */
    gboolean ddc_enable;
    gchar *ddc_file;

    /* < private > */
    GMutex lock;
    DirectForm2 **df441, **df48, **dfResampled, **sosPointer;
    int sosCount, resampledSOSCount, usedSOSCount;
    int samplerate;
};

struct _GstviperddcClass {
    GstAudioFilterClass parent_class;
};

GType gst_viperddc_get_type (void);

G_END_DECLS

#endif //GST_PLUGIN_VIPERDDC_GSTVIPERDDC_H
