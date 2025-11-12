# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\kylin-messenger-core_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\kylin-messenger-core_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\kylin-messenger_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\kylin-messenger_autogen.dir\\ParseCache.txt"
  "kylin-messenger-core_autogen"
  "kylin-messenger_autogen"
  )
endif()
