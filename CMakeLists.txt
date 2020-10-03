cmake_minimum_required(VERSION 3.10)
project(gst-plugin-viperddc C)

include_directories(.)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GST REQUIRED
        gstreamer-1.0>=1.0
        gstreamer-base-1.0>=1.0
        gstreamer-controller-1.0>=1.0
        gstreamer-audio-1.0>=1.0)

pkg_check_modules(GLIB REQUIRED
        glib-2.0
        gio-unix-2.0)

file(GLOB SOURCES
        *.h
        *.c)

add_library(gst-plugin-viperddc SHARED ${SOURCES} gstviperddc.h gstviperddc.c vdc.h)

target_include_directories(gst-plugin-viperddc PUBLIC ${GST_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS})
target_compile_options(gst-plugin-viperddc PUBLIC ${GST_CFLAGS} ${GLIB_CFLAGS})
target_link_libraries(gst-plugin-viperddc ${GST_LIBRARIES} ${GLIB_LIBRARIES})

set_target_properties(gst-plugin-viperddc PROPERTIES OUTPUT_NAME "gstviperddc")
