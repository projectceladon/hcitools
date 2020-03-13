LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := hcitool
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := hcitool.c
LOCAL_SRC_FILES += lib/bluetooth.c lib/hci.c lib/sdp.c
LOCAL_CFLAGS += -Wno-unused-parameter -Wno-pointer-arith \
                -Wno-missing-field-initializers \
                -fno-strict-overflow -fno-delete-null-pointer-checks \
                -fwrapv \
                -D_FORTIFY_SOURCE=2 \
                -fstack-protector-strong \
                -Wno-conversion-null \
                -Wnull-dereference \
                -Werror \
                -Warray-bounds \
                -Wformat -Wformat-security \
                -Werror=format-security

LOCAL_C_INCLUDES := $(KERNEL_HEADERS) $(LOCAL_PATH)/lib
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := hciconfig
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := hciconfig.c csr.c
LOCAL_SRC_FILES += lib/bluetooth.c lib/hci.c lib/sdp.c
LOCAL_CFLAGS += -Wno-unused-parameter -Wno-pointer-arith \
                -Wno-missing-field-initializers \
                -fno-strict-overflow -fno-delete-null-pointer-checks \
                -fwrapv \
                -D_FORTIFY_SOURCE=2 \
                -fstack-protector-strong \
                -Wno-conversion-null \
                -Wnull-dereference \
                -Werror \
                -Warray-bounds \
                -Wformat -Wformat-security \
                -Werror=format-security

LOCAL_C_INCLUDES := $(KERNEL_HEADERS) $(LOCAL_PATH)/lib
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_MODULE := btmon
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := monitor/main.c monitor/mainloop.c \
		monitor/display.c  monitor/hcidump.c \
		monitor/btsnoop.c monitor/control.c \
		monitor/packet.c monitor/vendor.c \
		monitor/lmp.c  monitor/l2cap.c \
		monitor/uuid.c monitor/sdp.c \
		monitor/crc.c monitor/ll.c
LOCAL_SRC_FILES += lib/bluetooth.c lib/hci.c lib/sdp.c
LOCAL_CFLAGS += -DVERSION=\"5.10\" \
                -Wno-unused-parameter -Wno-pointer-arith \
                -Wno-missing-field-initializers \
                -fno-strict-overflow -fno-delete-null-pointer-checks \
                -fwrapv \
                -D_FORTIFY_SOURCE=2 \
                -fstack-protector-strong \
                -Wno-conversion-null \
                -Wnull-dereference \
                -Werror \
                -Warray-bounds \
                -Wformat -Wformat-security \
                -Werror=format-security

LOCAL_C_INCLUDES := $(KERNEL_HEADERS) $(LOCAL_PATH)/lib
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_MODULE := hciattach
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SRC_FILES := hciattach.c \
		hciattach_st.c hciattach_ti.c \
		hciattach_tialt.c hciattach_ath3k.c \
		hciattach_qualcomm.c hciattach_intel.c \
		hciattach_bcm43xx.c hciattach_rtk.c
LOCAL_SRC_FILES += lib/bluetooth.c lib/hci.c lib/sdp.c
LOCAL_CFLAGS += -DFIRMWARE_DIR=\"/vendor/firmware\" \
                -Wno-unused-parameter -Wno-pointer-arith \
                -Wno-missing-field-initializers \
                -fno-strict-overflow -fno-delete-null-pointer-checks \
                -fwrapv \
                -Wno-for-loop-analysis \
                -D_FORTIFY_SOURCE=2 \
                -fstack-protector-strong \
                -Wno-conversion-null \
                -Wnull-dereference \
                -Werror \
                -Warray-bounds \
                -Wformat -Wformat-security \
                -Werror=format-security

LOCAL_C_INCLUDES := $(KERNEL_HEADERS) $(LOCAL_PATH)/lib
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
