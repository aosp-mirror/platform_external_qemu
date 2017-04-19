list(APPEND SOURCES
	DelimitedSerialization.cpp
	ProtobufLogging.cpp
	)

add_library(protobuf ${SOURCES})

