list(APPEND SOURCES
	SimAccessRules.cpp
	TagLengthValue.cpp
	#TagLengthValue_unittest.cpp
	debug.c
	gsm.c
	#gsm_unittest.cpp
	modem.c
	modem_driver.c
	#modem_unittest.cpp
	remote_call.c
	sim_card.c
	sms.c
	#sms_unittest.cpp
	sysdeps.c
	)


add_library(telephony ${SOURCES})


