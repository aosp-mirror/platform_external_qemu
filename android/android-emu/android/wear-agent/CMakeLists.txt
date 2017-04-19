list(APPEND SOURCES
	testing/WearAgentTestUtils.cpp
	android_wear_agent.cpp
	dummy.c
	PairUpWearPhone.cpp
	# PairUpWearPhone_unittest.cpp
	WearAgent.cpp
	#WearAgent_unittest.cpp
	)

# We need the checked in config-host
add_library(wear-agent ${SOURCES})

