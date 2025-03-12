# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/server-v0_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/server-v0_autogen.dir/ParseCache.txt"
  "server-v0_autogen"
  )
endif()
