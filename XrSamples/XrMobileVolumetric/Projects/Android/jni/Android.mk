LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := xrmobilevolumetric

include ../../../../cflags.mk

APP_STL := gnustl_static

LOCAL_C_INCLUDES := \
  					$(LOCAL_PATH)/../../../../../3rdParty/khronos/openxr/OpenXR-SDK/include \

LOCAL_CFLAGS += -I$(LOCAL_PATH)/../../../../../glm/

FILE_LIST := $(wildcard $(LOCAL_PATH)/../../../src/*.cpp)
LOCAL_SRC_FILES := $(FILE_LIST:$(LOCAL_PATH)/%=%)

# include default libraries
LOCAL_LDLIBS 			:= -llog -landroid -lGLESv3 -lEGL

LOCAL_CFLAGS += -UNDEBUG

LOCAL_STATIC_LIBRARIES := android_native_app_glue
LOCAL_SHARED_LIBRARIES := openxr_loader

LOCAL_SANITIZE := alignment bounds null unreachable integer
LOCAL_SANITIZE_DIAG := alignment bounds null unreachable integer
SANITIZE_TARGET:=hwaddress

include $(BUILD_SHARED_LIBRARY)

$(call import-module,OpenXR/Projects/AndroidPrebuilt/jni)
$(call import-module,android/native_app_glue)