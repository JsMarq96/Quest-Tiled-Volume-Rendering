LOCAL_PATH := $(call my-dir)

# Load LibZip
include $(CLEAR_VARS)
LOCAL_MODULE := libzip
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../../3rdParty/libzip/libs/$(TARGET_ARCH_ABI)/libzip.so
include $(PREBUILT_SHARED_LIBRARY)

# Load STB (image)
include $(CLEAR_VARS)
LOCAL_MODULE := libstb
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../../3rdParty/stb/lib/android/$(TARGET_ARCH_ABI)/libstb.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := xrmobilevolumetric

include ../../../../cflags.mk

APP_STL := gnustl_static
LOCAL_C_INCLUDES := \
  					$(LOCAL_PATH)/../../../../../3rdParty/khronos/openxr/OpenXR-SDK/include \

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../../3rdParty/libzip/jni
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../../../3rdParty/stb/src

LOCAL_CFLAGS += -I$(LOCAL_PATH)/../../../../../glm/

FILE_LIST := $(wildcard $(LOCAL_PATH)/../../../src/*.cpp)
LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)

# include default libraries
LOCAL_LDLIBS 			:= -llog -landroid -lGLESv3 -lEGL

#LOCAL_CFLAGS += -UNDEBUG -g

LOCAL_STATIC_LIBRARIES := android_native_app_glue
LOCAL_STATIC_LIBRARIES += libstb
LOCAL_SHARED_LIBRARIES := openxr_loader
LOCAL_SHARED_LIBRARIES += libzip

LOCAL_SANITIZE := alignment bounds null unreachable integer
LOCAL_SANITIZE_DIAG := alignment bounds null unreachable integer
SANITIZE_TARGET:= hwaddress

include $(BUILD_SHARED_LIBRARY)

$(call import-module,OpenXR/Projects/AndroidPrebuilt/jni)
$(call import-module,android/native_app_glue)
