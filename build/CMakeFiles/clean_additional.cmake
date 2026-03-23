# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "CMakeFiles\\ai_verif_studio_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\ai_verif_studio_autogen.dir\\ParseCache.txt"
  "ai_verif_studio_autogen"
  )
endif()
