LOCAL_BUILT_MODULE_SYM := $(LOCAL_BUILT_MODULE).sym
$(LOCAL_BUILT_MODULE_SYM): PRIVATE_SYMTOOL := $(LOCAL_SYMTOOL)
$(LOCAL_BUILT_MODULE_SYM): $(LOCAL_BUILT_MODULE)
	@ echo "Symbol file: $@"
	@ $(PRIVATE_SYMTOOL) $< > $@ && \
	SYMB_CODE=`head -n1 $@ | cut -d" " -f4` && \
	SYMB_NAME=`head -n1 $@ | cut -d" " -f5` && \
	mkdir -p $(SYMBOLS_DIR)/$$SYMB_NAME/$$SYMB_CODE && \
	cp $@ $(SYMBOLS_DIR)/$$SYMB_NAME/$$SYMB_CODE
ifneq ($(BUILD_DEBUG_EMULATOR),true)
	@ echo "Stripping $<"
ifeq ($(LOCAL_STRIP_SHARED), true)
	strip -x "$<"
else
	strip "$<"
endif
endif


$(if $(LOCAL_GENERATE_SYMBOLS), $(eval SYMBOLS+=$(LOCAL_BUILT_MODULE_SYM)))
