ifeq (true,$(LOCAL_GENERATE_SYMBOLS))
$(eval $(call install-symbol,$(LOCAL_BUILT_MODULE)))
endif
