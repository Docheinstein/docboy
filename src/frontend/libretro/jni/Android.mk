LOCAL_PATH := $(call my-dir)

SOURCE_DIR   := $(LOCAL_PATH)/../../..
LIBRETRO_DIR   := $(SOURCE_DIR)/frontend/libretro

include $(LIBRETRO_DIR)/Makefile.common

CXXFLAGS := -DHAVE_STDINT_H -DHAVE_INTTYPES_H -D__LIBRETRO__

include $(CLEAR_VARS)
LOCAL_MODULE    := retro
LOCAL_C_INCLUDES := $(SOURCE_DIR)
LOCAL_SRC_FILES := $(SOURCES_CXX)
LOCAL_CXXFLAGS  := $(CXXFLAGS)
LOCAL_LDFLAGS   := -Wl,-version-script=$(LIBRETRO_DIR)/link.T
include $(BUILD_SHARED_LIBRARY)

