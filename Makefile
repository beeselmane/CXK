ifeq ($(VERBOSE),YES)
	v :=
else
	v := @
endif

TYPE ?= development

ifeq ($(TYPE),release)
	TYPE_DEFINE=-DkCXRelease=1
else ifeq ($(TYPE),development)
	TYPE_DEFINE=-DkCXDevelopment=1
else ifeq ($(TYPE),debug)
	TYPE_DEFINE=-DkCXDebug=1
endif

TOOLCHAIN_EFI   ?= x86_64-pe-
TOOLCHAIN       ?= x86_64-elf-

AS              := $(TOOLCHAIN)gcc -x assembler-with-cpp
CC              := $(TOOLCHAIN)gcc
LD              := $(TOOLCHAIN)gcc
AR              := $(TOOLCHAIN)ar
OBJCOPY         := $(TOOLCHAIN)objcopy
EFI_AS          := $(TOOLCHAIN)gcc -x assembler-with-cpp
EFI_CC          := $(TOOLCHAIN)gcc
EFI_LD          := $(TOOLCHAIN)gcc
EFI_OBJCOPY     := $(TOOLCHAIN_EFI)objcopy

PRINT           := @/bin/echo -e
MKDIR           := mkdir -p
RM              := rm -rdf

SOURCE          ?= $(shell pwd)
BUILD_ROOT      ?= $(SOURCE)/build
OBJECTS         ?= $(BUILD_ROOT)/obj
OUTPUT          ?= $(BUILD_ROOT)/out-$(TYPE)
SHARED_OBJ      := $(OBJECTS)/shared-$(TYPE)
LOADER_OBJ      := $(OBJECTS)/loader-$(TYPE)
KERNEL_OBJ      := $(OBJECTS)/kernel-$(TYPE)
DIRS            := $(OBJECTS) $(OUTPUT) $(SHARED_OBJ) $(LOADER_OBJ) $(KERNEL_OBJ)

SHARED          := $(SOURCE)/SharedCode
LOADER          := $(SOURCE)/boot
KERNEL          := $(SOURCE)/kernel
INCLUDE         := $(SOURCE)/include

INCLUDES        := -I$(INCLUDE)
DEFINES         := $(TYPE_DEFINE)
# -target x86_64--none-elf
CCFLAGS         := -ffreestanding -nostdinc -nostdlib -Wall -Wextra     \
                   -Wno-unused-parameter -Wno-unused-variable           \
                   -Wno-pointer-sign -Wno-incompatible-pointer-types    \
                   -fno-exceptions -fPIC -mno-red-zone $(INCLUDES) $(DEFINES)
LDFLAGS         := -ffreestanding -nostdlib -fPIC
ASMFLAGS        := -Wall -Wextra $(INCLUDES) $(DEFINES) -DkCXAssemblyCode=1

DEFAULT_COLOR   = \\e[39m
BLACK           = \\e[30m
RED             = \\e[31m
GREEN           = \\e[32m
YELLOW          = \\e[33m
BLUE            = \\e[34m
MAGENTA         = \\e[35m
CYAN            = \\e[36m
LIGHT_GRAY      = \\e[37m
DARK_GRAY       = \\e[90m
LIGHT_RED       = \\e[91m
LIGHT_GREEN     = \\e[92m
LIGHT_YELLOW    = \\e[93m
LIGHT_BLUE      = \\e[94m
LIGHT_MAGENTA   = \\e[95m
LIGHT_CYAN      = \\e[96m
WHITE           = \\e[97m

.DEFAULT: all

.PHONY: all
.PHONY: clean
.PHONY: clean-objects

include headers.mk
include shared.mk

include $(LOADER)/loader.mk
include $(KERNEL)/kernel.mk

all: $(OBJECTS) $(OUTPUT) shared loader kernel

clean-objects:
	$(PRINT) $(YELLOW)Remove: $(OBJECTS)...$(DEFAULT_COLOR)
	$(v)$(RM) $(OBJECTS)

clean: clean-objects
	$(PRINT) $(YELLOW)Remove: $(OUTPUT)...$(DEFAULT_COLOR)
	$(v)$(RM) $(OUTPUT)
	$(PRINT) $(YELLOW)Remove: $(BUILD_ROOT)...$(DEFAULT_COLOR)
	$(v)$(RM) $(BUILD_ROOT)

$(DIRS): %:
	$(PRINT) $(MAGENTA)Directory $@...$(DEFAULT_COLOR)
	$(v)$(MKDIR) $@

#$(OUTPUT)/$(SETUP): SLSetupKernel.ld $(OBJECTS)/SLSetupEnvironment.o $(OBJECTS)/SLSetupFinalize.o $(OUTPUT)/$(SHLIB)
#	@echo Linking $(@F)...
#	$(v)$(CC) -T $< -o $@ -ffreestanding -nostdlib $(OBJECTS)/SLSetupEnvironment.o $(OBJECTS)/SLSetupFinalize.o -L$(OUTPUT) -l:$(SHLIB)

#$(OBJECTS)/SLSetupFinalize.o: SLSetupFinalize.c
#	@echo Compiling $(<F)...
#	$(v)$(CC) -c $< -o $@ $(CCFLAGS)

#$(OBJECTS)/SLSetupEnvironment.o: SLSetupEnvironment.s
#	@echo Compiling $(<F)...
#	$(v)$(ASM) -c $< -o $@ $(ASMFLAGS)
