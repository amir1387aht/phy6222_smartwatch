##############################################################################
PROJECT_NAME ?= phy6222_watch
PROJECT_DEFS ?= -DDEVICE=DEVICE_WATCH_1
BLE ?= 1
##############################################################################
COM_PORT = COM19
COM_SPEED = 115200
UART_LOG_BPS = 115200

##############################################################################
# Source and Libs Directories
SRC_PATH := source
LIB_PATH := libs

# Collect C and C++ files from both source and libs folders
SRC_C := $(shell find $(SRC_PATH) -type f -name "*.c")
SRC_CPP := $(shell find $(SRC_PATH) -type f -name "*.cpp")
LIB_C := $(shell find $(LIB_PATH) -type f -name "*.c")
LIB_CPP := $(shell find $(LIB_PATH) -type f -name "*.cpp")

# Update VPATH to include the new libs folder (in addition to source and SDK)
VPATH := $(SRC_PATH) $(LIB_PATH) $(SDK_PATH)

# Add the base include directories for source and libs
INCLUDES = -I$(SRC_PATH) -I$(LIB_PATH)

# Recursive wildcard function to find all subdirectories for header files
rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))
ALL_DIRS := $(sort $(dir $(call rwildcard,$(SRC_PATH),*)))
ALL_LIB_DIRS := $(sort $(dir $(call rwildcard,$(LIB_PATH),*)))

# Append all discovered include directories (excluding the top-level ones already added)
INCLUDES += $(patsubst %,-I%,$(filter-out $(SRC_PATH)/,$(ALL_DIRS)))
INCLUDES += $(patsubst %,-I%,$(filter-out $(LIB_PATH)/,$(ALL_LIB_DIRS)))

# Combine all source files for compilation
SRCS = $(SRC_C) $(SRC_CPP) $(LIB_C) $(LIB_CPP)

##############################################################################
DEFINES += -D__GCC
DEFINES += $(PROJECT_DEFS)
DEFINES += -DCFG_SLEEP_MODE=PWR_MODE_SLEEP
#DEFINES += -DCFG_SLEEP_MODE=PWR_MODE_NO_SLEEP
DEFINES += -DDEBUG_INFO=3
DEFINES += -DMTU_SIZE=240
DEFINES += -DMAX_NUM_LL_CONN=1
DEFINES += -DDEF_GAPBOND_MGR_ENABLE=0

#Debug:
DEFINES += -DTEST_RTC_DELTA=1
DEFINES += -DLL_DEBUG_NONE=1 
DEFINES += -DCLK_16M_ONLY=1
DEFINES += -DSTACK_MAX_SRAM=1
#if Flash HS
#DEFINES += -DXFLASH_HIGH_SPEED=1

#HOST_CONFIG:
DEFINES += -DBROADCASTER_CFG=0x01 
DEFINES += -DOBSERVER_CFG=0x02 
DEFINES += -DPERIPHERAL_CFG=0x04 
DEFINES += -DCENTRAL_CFG=0x08 
DEFINES += -DHOST_CONFIG=0x04

DEFINES += -DARMCM0
DEFINES += -DOSAL_CBTIMER_NUM_TASKS=1 
#  osal_cbtimer.h
DEFINES += -DENABLE_LOG_ROM_=0

#osal_heap info:
DEFINES += -DOSALMEM_METRICS=0

DEFINES += -DPHY_MCU_TYPE=MCU_BUMBEE_M0

#osal_snv.c:
DEFINES += -DUSE_FS=0

#Not used:
#DEFINES += -D_RTE_
#DEFINES += -DHCI_TL_NONE=1
#DEFINES += -D_OBJ_DIR_FOR_DTM_=0
#DEFINES += -DDBG_ROM_MAIN=0
#DEFINES += -DOSALMEM_METRICS=0
#DEFINES += -DAPP_CFG=0
#DEFINES += -DCFG_CP
#CTRL_CONFIG:
#DEFINES += -DADV_NCONN_CFG=0x01
#DEFINES += -DADV_CONN_CFG=0x02
#DEFINES += -DSCAN_CFG=0x04
#DEFINES += -DINIT_CFG=0x08

##############################################################################
BIN_DIR = ./bin
OBJ_DIR = ./build
SDK_PATH = ./sdk
PYTHON = python
GCC_PATH = 
CC = $(GCC_PATH)arm-none-eabi-gcc
OBJCOPY = $(GCC_PATH)arm-none-eabi-objcopy
OBJDUMP = $(GCC_PATH)arm-none-eabi-objdump
SIZE = $(GCC_PATH)arm-none-eabi-size
READELF = $(GCC_PATH)arm-none-eabi-readelf
CXX = $(GCC_PATH)arm-none-eabi-g++
##############################################################################
ARCH_FLAGS := -mcpu=cortex-m0 -mthumb -mthumb-interwork
OPT_CFLAGS ?= -Os
DEB_CFLAGS ?= -g3 -ggdb
##############################################################################
ASFLAGS	   := $(ARCH_FLAGS) $(OPT_CFLAGS) $(DEB_CFLAGS)
CFLAGS     := $(ARCH_FLAGS) $(OPT_CFLAGS) $(DEB_CFLAGS)
CFLAGS     += -W -Wall --std=gnu99
CFLAGS     += -fno-diagnostics-show-caret
CFLAGS     += -fdata-sections -ffunction-sections
CFLAGS     += -funsigned-char -funsigned-bitfields
#CFLAGS     += -fpack-struct
#CFLAGS     += -mno-unaligned-access
#CFLAGS     += -munaligned-access
CFLAGS     += -fms-extensions
CFLAGS     += -specs=nosys.specs
CFLAGS     += -Wl,--gc-sections

CXXFLAGS := $(ARCH_FLAGS) $(OPT_CFLAGS) $(DEB_CFLAGS)
CXXFLAGS += -W -Wall --std=gnu++11
CXXFLAGS += -fno-exceptions -fno-rtti
CXXFLAGS += -fdata-sections -ffunction-sections
CXXFLAGS += -fno-diagnostics-show-caret
CXXFLAGS += -funsigned-char -funsigned-bitfields
CXXFLAGS += -fms-extensions
CXXFLAGS += -specs=nosys.specs
CXXFLAGS += -Wl,--gc-sections

ifdef BOOT_OTA
LDSCRIPT   ?= $(SDK_PATH)/misc/boot_ota_phy62x2.ld
DEFINES += -DOTA_TYPE=OTA_TYPE_BOOT
BIN_OTA	= $(OBJ_DIR)/$(PROJECT_NAME).bin
ADD_OPT = -w 0x2F00 -f ota_upboot.add
else
LDSCRIPT   ?= $(SDK_PATH)/misc/phy6222.ld
DEFINES += -DOTA_TYPE=OTA_TYPE_NONE
BIN_OTA	= $(OBJ_DIR)/$(PROJECT_NAME).bin
ADD_OPT =
endif

LDFLAGS    := $(ARCH_FLAGS)
LDFLAGS    += --static -nostartfiles -nostdlib
LDFLAGS    += -Wl,--gc-sections
LDFLAGS    += -Wl,--script=$(LDSCRIPT)
# LDFLAGS    += -Wl,--no-warn-rwx-segments
LDFLAGS    += -Wl,--just-symbols=$(SDK_PATH)/misc/bb_rom_sym_m0.gcc
LDFLAGS    += -Wl,-Map=$(OBJ_DIR)/$(PROJECT_NAME).map 
LIBS       += -Wl,--start-group -lgcc -lnosys -Wl,--end-group

##############################################################################
INCLUDES += -I$(SDK_PATH)/misc
INCLUDES += -I$(SDK_PATH)/misc/CMSIS/include
INCLUDES += -I$(SDK_PATH)/components/arch/cm0
INCLUDES += -I$(SDK_PATH)/components/inc
INCLUDES += -I$(SDK_PATH)/components/osal/include
INCLUDES += -I$(SDK_PATH)/components/common
INCLUDES += -I$(SDK_PATH)/components/ble/controller
INCLUDES += -I$(SDK_PATH)/components/ble/include
INCLUDES += -I$(SDK_PATH)/components/ble/hci
INCLUDES += -I$(SDK_PATH)/components/ble/host
INCLUDES += -I$(SDK_PATH)/components/profiles/ota_app
INCLUDES += -I$(SDK_PATH)/components/profiles/DevInfo
INCLUDES += -I$(SDK_PATH)/components/profiles/SimpleProfile
INCLUDES += -I$(SDK_PATH)/components/profiles/Roles
INCLUDES += -I$(SDK_PATH)/components/driver/adc
INCLUDES += -I$(SDK_PATH)/components/driver/bsp_button
INCLUDES += -I$(SDK_PATH)/components/driver/clock
INCLUDES += -I$(SDK_PATH)/components/driver/dma
INCLUDES += -I$(SDK_PATH)/components/driver/flash
INCLUDES += -I$(SDK_PATH)/components/driver/gpio
INCLUDES += -I$(SDK_PATH)/components/driver/spi
INCLUDES += -I$(SDK_PATH)/components/driver/i2c
INCLUDES += -I$(SDK_PATH)/components/driver/key
INCLUDES += -I$(SDK_PATH)/components/driver/kscan
INCLUDES += -I$(SDK_PATH)/components/driver/led_light
INCLUDES += -I$(SDK_PATH)/components/driver/log
INCLUDES += -I$(SDK_PATH)/components/driver/pwm
INCLUDES += -I$(SDK_PATH)/components/driver/pwrmgr
INCLUDES += -I$(SDK_PATH)/components/driver/qdec
INCLUDES += -I$(SDK_PATH)/components/driver/spi
INCLUDES += -I$(SDK_PATH)/components/driver/dma
INCLUDES += -I$(SDK_PATH)/components/driver/spiflash
INCLUDES += -I$(SDK_PATH)/components/driver/timer
INCLUDES += -I$(SDK_PATH)/components/driver/uart
INCLUDES += -I$(SDK_PATH)/components/driver/voice
INCLUDES += -I$(SDK_PATH)/components/driver/watchdog
INCLUDES += -I$(SDK_PATH)/components/libraries/crc16
INCLUDES += -I$(SDK_PATH)/components/libraries/cliface
#INCLUDES += -I$(SDK_PATH)/components/libraries/fs
INCLUDES += -I$(SDK_PATH)/components/driver/watchdog

###########################################
# LIBs
SRCS += $(SDK_PATH)/lib/rf/patch.c
SRCS += $(SDK_PATH)/lib/sec/phy_sec_ext.c
SRCS += $(SDK_PATH)/lib/sec/aes.c
ifdef BLE
SRCS += $(SDK_PATH)/lib/ble_host/att_client.c
SRCS += $(SDK_PATH)/lib/ble_host/att_server.c
SRCS += $(SDK_PATH)/lib/ble_host/att_util.c
SRCS += $(SDK_PATH)/lib/ble_host/gap_centdevmgr.c
SRCS += $(SDK_PATH)/lib/ble_host/gap_centlinkmgr.c
SRCS += $(SDK_PATH)/lib/ble_host/gap_configmgr.c
SRCS += $(SDK_PATH)/lib/ble_host/gap_devmgr.c
SRCS += $(SDK_PATH)/lib/ble_host/gap_linkmgr.c
SRCS += $(SDK_PATH)/lib/ble_host/gap_peridevmgr.c
SRCS += $(SDK_PATH)/lib/ble_host/gap_perilinkmgr.c
#SRCS += $(SDK_PATH)/lib/ble_host/gap_simpletask.c
SRCS += $(SDK_PATH)/lib/ble_host/gap_task.c
SRCS += $(SDK_PATH)/lib/ble_host/gatt_client.c
SRCS += $(SDK_PATH)/lib/ble_host/gatt_server.c
SRCS += $(SDK_PATH)/lib/ble_host/gatt_task.c
SRCS += $(SDK_PATH)/lib/ble_host/gatt_uuid.c
SRCS += $(SDK_PATH)/lib/ble_host/l2cap_if.c
SRCS += $(SDK_PATH)/lib/ble_host/l2cap_task.c
SRCS += $(SDK_PATH)/lib/ble_host/l2cap_util.c
SRCS += $(SDK_PATH)/lib/ble_host/linkdb.c
SRCS += $(SDK_PATH)/lib/ble_host/sm_intpairing.c
SRCS += $(SDK_PATH)/lib/ble_host/sm_mgr.c
SRCS += $(SDK_PATH)/lib/ble_host/sm_pairing.c
SRCS += $(SDK_PATH)/lib/ble_host/smp.c
SRCS += $(SDK_PATH)/lib/ble_host/sm_rsppairing.c
SRCS += $(SDK_PATH)/lib/ble_host/sm_task.c
endif
ifdef LIB_BLE_ADD
SRCS += $(SDK_PATH)/lib/ble_controller/ll_common.c
SRCS += $(SDK_PATH)/lib/ble_controller/ll_enc.c
SRCS += $(SDK_PATH)/lib/ble_controller/ll_hw_drv.c
SRCS += $(SDK_PATH)/lib/ble_controller/ll_hwItf.c
SRCS += $(SDK_PATH)/lib/ble_controller/ll.c
SRCS += $(SDK_PATH)/lib/ble_controller/ll_masterEndCauses.c
SRCS += $(SDK_PATH)/lib/ble_controller/ll_slaveEndCauses.c
SRCS += $(SDK_PATH)/lib/ble_controller/ll_sleep.c
endif
SRCS += $(SDK_PATH)/lib/ble_controller/rf_phy_driver.c

###########################################
# SDK
SRCS += $(SDK_PATH)/components/driver/clock/clock.c
SRCS += $(SDK_PATH)/components/driver/flash/flash.c
SRCS += $(SDK_PATH)/components/driver/gpio/gpio.c
SRCS += $(SDK_PATH)/components/driver/spi/spi.c
SRCS += $(SDK_PATH)/components/driver/dma/dma.c
#SRCS += $(SDK_PATH)/components/driver/key/key.c
#SRCS += $(SDK_PATH)/components/driver/led_light/led_light.c
#SRCS += $(SDK_PATH)/components/driver/pwm/pwm.c
SRCS += $(SDK_PATH)/components/driver/pwrmgr/pwrmgr.c
SRCS += $(SDK_PATH)/components/driver/timer/timer.c
SRCS += $(SDK_PATH)/components/driver/uart/uart.c
SRCS += $(SDK_PATH)/components/driver/watchdog/watchdog.c
SRCS += $(SDK_PATH)/components/driver/log/my_printf.c

# SRCS += $(SDK_PATH)/components/profiles/Roles/central.c
# SRCS += $(SDK_PATH)/components/profiles/ota_app/ota_app_service.c
# SRCS += $(SDK_PATH)/components/profiles/Roles/peripheral.c
# SRCS += $(SDK_PATH)/components/profiles/Roles/gap.c
# SRCS += $(SDK_PATH)/components/profiles/Roles/gapbondmgr.c
# SRCS += $(SDK_PATH)/components/profiles/Roles/gapgattserver.c
# SRCS += $(SDK_PATH)/components/profiles/GATT/gattservosal_init.c
# SRCS += $(SDK_PATH)/components/profiles/DevInfo/devinfoservice.c

# SRCS += $(SDK_PATH)/components/osal/snv/osal_snv.c
#SRCS += $(SDK_PATH)/components/libraries/fs/fs.c

SRCS += $(SDK_PATH)/misc/jump_table.c

##############################################################################

INCLUDES += -I$(SDK_PATH)/misc/CMSIS/device/phyplus

STARTUP_ASM = $(SDK_PATH)/misc/CMSIS/device/phyplus/phy6222_start.s

SRCS += $(SDK_PATH)/misc/CMSIS/device/phyplus/phy6222_cstart.c
SRCS += $(SDK_PATH)/misc/CMSIS/device/phyplus/phy6222_vectors.c

##############################################################################

CFLAGS  += $(DEFINES) $(INCLUDES)
CXXFLAGS += $(DEFINES) $(INCLUDES)

##############################################################################
# Generate object file paths properly
C_OBJS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRCS))
CPP_OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC_CPP))
ASM_OBJS = $(patsubst %.s,$(OBJ_DIR)/%.o,$(STARTUP_ASM))

OBJS = $(C_OBJS) $(CPP_OBJS) $(ASM_OBJS)

DEPENDENCY_LIST = $(OBJS:%o=%d)

##############################################################################
.PHONY: all directory clean size flash erase_and_flash

all: directory $(OBJ_DIR)/$(PROJECT_NAME).elf $(OBJ_DIR)/$(PROJECT_NAME).hex $(BIN_OTA) $(OBJ_DIR)/$(PROJECT_NAME).asm size flash terminal

$(OBJ_DIR)/$(PROJECT_NAME).elf $(OBJ_DIR)/$(PROJECT_NAME).map: $(OBJS) $(LDSCRIPT) Makefile
	@echo LD: $@
	@$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $(OBJ_DIR)/$(PROJECT_NAME).elf

%.hex: %.elf
	@echo OBJCOPY: $@
	@$(OBJCOPY) -O ihex $^ $@

%.bin: %.hex
	@echo Make: $@
	@$(PYTHON) ./phy62x2_ota.py $(ADD_OPT) $(OBJ_DIR)/$(PROJECT_NAME).hex

%.asm: %.elf
	@echo OBJDUMP: $@
	@$(OBJDUMP) -s -S $^ >$@ 

$(OBJ_DIR)/%.o: %.c
	@echo CC: $<
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@
	@$(CC) -MM $(CFLAGS) $< -MT $@ -MF $(patsubst %.o,%.d,$@)

$(OBJ_DIR)/%.o: %.cpp
	@echo CXX: $<
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@$(CXX) -MM $(CXXFLAGS) $< -MT $@ -MF $(patsubst %.o,%.d,$@)

$(OBJ_DIR)/%.o: %.s
	@echo AS: $<
	@mkdir -p $(dir $@)
	@$(CC) $(ASFLAGS) -c $< -o $@
	@$(CC) -MM $(ASFLAGS) $< -MT $@ -MF $(patsubst %.o,%.d,$@)

flash: $(OBJ_DIR)/$(PROJECT_NAME).hex
	@$(PYTHON) ./rdwr_phy62x2.py -p$(COM_PORT) -b $(COM_SPEED) -r wh $(OBJ_DIR)/$(PROJECT_NAME).hex

terminal:
	@$(PYTHON) ./miniterm.py $(COM_PORT) $(UART_LOG_BPS) --rts 1 --rtstoggle 100 --exit-char 3 --rtsexit 1

flash_ota:
	@$(PYTHON) ./rdwr_phy62x2.py -p$(COM_PORT) -b $(COM_SPEED) -r we 0x10000 $(BIN_OTA)

identify:
	@$(PYTHON) ./rdwr_phy62x2.py -p$(COM_PORT) -b $(COM_SPEED) i

erase_and_flash:
	@$(PYTHON) ./rdwr_phy62x2.py -p$(COM_PORT) -b $(COM_SPEED) -e -r wh $(OBJ_DIR)/$(PROJECT_NAME).hex

reset:
	@$(PYTHON) ./rdwr_phy62x2.py -p$(COM_PORT) -r i

directory:
	@mkdir -p $(OBJ_DIR)

size: $(OBJ_DIR)/$(PROJECT_NAME).elf
	@echo size:
	@$(SIZE) -t $^
	@$(READELF) -l $^
	@echo

clean:
	@echo clean
	@-rm -rf $(OBJ_DIR)

-include $(DEPENDENCY_LIST)