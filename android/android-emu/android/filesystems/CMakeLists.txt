include_directories(${EXT4_UTILS_INCLUDE_DIRS})

list(APPEND SOURCES
  ext4_resize.cpp
  # ext4_resize_unittest.cpp
  ext4_utils.cpp
  #ext4_utils_unittest.cpp
  fstab_parser.cpp
  #fstab_parser_unittest.cpp
  internal/PartitionConfigBackend.cpp
  partition_config.cpp
  #partition_config_unittest.cpp
  partition_types.cpp
  # partition_types_unittest.cpp
  ramdisk_extractor.cpp
  # ramdisk_extractor_unittest.cpp
  testing/TestSupport.cpp
  )




add_library(filesystems ${SOURCES})


