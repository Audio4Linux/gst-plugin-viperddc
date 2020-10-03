# gst-plugin-viperddc
Open-source VDC engine for GStreamer1

This is a standalone VDC engine for use with GStreamer1.
It does not require any extra dependencies.

The digital biquad filter processor code is based on the [open-source version of JamesDSP](https://github.com/james34602/JamesDSPManager/blob/master/Open_source_edition/Audio_Engine/eclipse_libjamesdsp_free_bp/jni/vdc.c) (GPLv2).

### Build from sources
Clone repository
```bash
git clone https://github.com/Audio4Linux/gst-plugin-viperddc
```

Build the shared library
```bash
cmake .
make
```

You should end up with `libgstviperddc.so`.
Copy the library into one of the GStreamer plugin paths and load it into your GStreamer server.

To verify whether it was installed correctly:
```
gst-inspect-1.0 viperddc
```
