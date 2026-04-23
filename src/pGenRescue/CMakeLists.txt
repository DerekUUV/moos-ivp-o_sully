#--------------------------------------------------------
# The CMakeLists.txt for:                     pGenRescue
# Author(s):                              DerekOSullivan
#--------------------------------------------------------

SET(SRC
  GenRescue.cpp
  GenRescue_Info.cpp
  main.cpp
)

ADD_EXECUTABLE(pGenRescue ${SRC})

TARGET_LINK_LIBRARIES(pGenRescue
   ${MOOS_LIBRARIES}
   apputil
   m
   geometry
   mbutil
   pthread)
