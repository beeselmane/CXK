LOADER_FILE      = CXSystemLoader.efi
LOADER_TARGET   := $(OUTPUT)/$(LOADER_FILE)

LOADER_C        := $(wildcard $(LOADER)/*.c)
LOADER_ASM      := $(wildcard $(LOADER)/*.s)
LOADER_C_OBJ    := $(LOADER_C:$(LOADER)/%.c=$(LOADER_OBJ)/%.o)
LOADER_ASM_OBJ  := $(LOADER_ASM:$(LOADER)/%.s=$(LOADER_OBJ)/%.o)

LOADER_LIBRARY  := $(OUTPUT)/$(LOADER_FILE:%.efi=%.so)
LOADER_LINKFILE := $(LOADER)/CXSystemLoader.ld

LOADER_OBJCOPY_FLAGS  = -j .peheader -j .text     -j .sdata   \
						-j .data     -j .dynamic \
						-j .dynsym   -j .rel     \
						-j .rela     -j .reloc   \
						-j .eh_frame             \
						--output-target=efi-app-x86_64
LOADER_CCFLAGS        = -mabi=ms -fno-stack-protector -fshort-wchar $(CCFLAGS) -DkCXBootloaderCode=1
LOADER_LDFLAGS        = -znocombreloc -Wl,-E -Wl,-Bsymbolic -shared $(LDFLAGS)

.PHONY: loader

loader: $(OBJECTS) $(OUTPUT) $(LOADER_OBJ) $(HEADERS) $(LOADER_TARGET)

$(LOADER_TARGET): $(LOADER_LIBRARY)
	$(PRINT) $(LIGHT_GRAY)EFI Application: $(@F)...$(DEFAULT_COLOR)
	$(v)$(EFI_OBJCOPY) $(LOADER_OBJCOPY_FLAGS) $< $@

$(LOADER_LIBRARY): $(LOADER_LINKFILE) $(LOADER_C_OBJ) $(LOADER_ASM_OBJ)
	$(PRINT) $(LIGHT_RED)Shared Library: $(@F)...$(DEFAULT_COLOR)
	$(v)$(EFI_LD) -Wl,-T$< -o $@ $(LOADER_C_OBJ) $(LOADER_ASM_OBJ) $(LOADER_LDFLAGS)

$(LOADER_C_OBJ): $(LOADER_OBJ)/%.o: $(LOADER)/%.c $(HEADERS)
	$(PRINT) $(LIGHT_GREEN)Target CC: $(<F)...$(DEFAULT_COLOR)
	$(v)$(EFI_CC) -c $< -o $@ $(LOADER_CCFLAGS)

$(LOADER_ASM_OBJ): $(LOADER_OBJ)/%.o: $(LOADER)/%.s $(HEADERS)
	$(PRINT) $(LIGHT_CYAN)Target Assembly: $(<F)...$(DEFAULT_COLOR)
	$(v)$(EFI_AS) -c $< -o $@ $(ASMFLAGS)
