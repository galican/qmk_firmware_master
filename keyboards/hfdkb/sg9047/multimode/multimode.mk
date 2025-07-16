THIS_MK_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
MULTIMODE_DIR := $(THIS_MK_DIR)

RGB_MATRIX_ENABLE ?= yes
ifeq ($(strip $(RGB_MATRIX_ENABLE)), yes)
    OPT_DEFS += -DRGB_MATRIX_BLINK_ENABLE
    SRC += $(MULTIMODE_DIR)/rgb_matrix_blink.c
endif

MULTIMODE_ENABLE ?= yes
ifeq ($(strip $(MULTIMODE_ENABLE)), yes)
    OPT_DEFS += -DMULTIMODE_ENABLE
    OPT_DEFS += -DENTRY_STOP_MODE
    OPT_DEFS += -DNO_USB_STARTUP_CHECK
    UART_DRIVER_REQUIRED = yes
    COMMON_VPATH += $(MULTIMODE_DIR)/
    SRC += $(MULTIMODE_DIR)/multimode.c
    SRC += $(MULTIMODE_DIR)/bt_task.c
    SRC += $(MULTIMODE_DIR)/retarget_suspend.c
    SRC += $(MULTIMODE_DIR)/lp_sleep.c
    LDFLAGS += -L $(MULTIMODE_DIR)/ -l_bts
endif
