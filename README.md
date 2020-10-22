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
Now you need to copy the file into one of GStreamer's plugin directories. It can be different between distros.

Debian:
```bash
sudo cp libgstviperfx.so /usr/lib/x86_64-linux-gnu/gstreamer-1.0/  
```
Arch:
```bash
sudo cp libgstviperfx.so /usr/lib/gstreamer-1.0/  
```

To verify whether it was installed correctly:
```
gst-inspect-1.0 viperddc
```

It is now installed. You can launch an audio processing pipeline using `gst-launch-1.0` or link it into your own GStreamer host application.

### Example pipeline
Play and process audio file 'test.mp3' using 'bass.vdc':
```bash
gst-launch-1.0 filesrc location="test.mp3" ! decodebin ! audioresample ! audioconvert ! viperddc ddc-enable="true" ddc-file="bass.vdc" ! autoaudiosink
```

You can use the `GST_DEBUG` environment variable to enable debug output:
```
export GST_DEBUG=viperddc:8
```