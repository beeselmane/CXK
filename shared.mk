LIBKERNEL        = libKernelShared.a
LIB_TARGET      := $(OUTPUT)/$(LIBKERNEL)

SHARED_C        := $(wildcard $(SHARED)/*.c)
SHARED_ASM      := $(wildcard $(SHARED)/*.s)
SHARED_C_OBJ    := $(SHARED_C:$(SHARED)/%.c=$(SHARED_OBJ)/%.o)
SHARED_ASM_OBJ  := $(SHARED_ASM:$(SHARED)/%.s=$(SHARED_OBJ)/%.o)

LDFLAGS         += -L$(OUTPUT) -l:$(LIBKERNEL)

.PHONY: shared

shared: $(OBJECTS) $(OUTPUT) $(SHARED_OBJ) $(HEADERS) $(LIB_TARGET)

$(LIB_TARGET): $(SHARED_C_OBJ) $(SHARED_ASM_OBJ)
	$(PRINT) $(LIGHT_RED)Static Library: $(@F)$(DEFAULT_COLOR)
	$(v)if [ -f $@ ]; then $(AR) r $@ $?; else $(AR) rcs $@ $^; fi

$(SHARED_C_OBJ): $(SHARED_OBJ)/%.o: $(SHARED)/%.c $(HEADERS)
	$(PRINT) $(LIGHT_GREEN)Target CC: $(<F)...$(DEFAULT_COLOR)
	$(v)$(CC) -c $< -o $@ $(CCFLAGS)

$(SHARED_ASM_OBJ): $(SHARED_OBJ)/%.o: $(SHARED)/%.s $(HEADERS)
	$(PRINT) $(LIGHT_CYAN)Target Assembly: $(<F)...$(DEFAULT_COLOR)
	$(v)$(AS) -c $< -o $@ $(ASMFLAGS)
