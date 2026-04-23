#--------------------------------------------------------
# The CMakeLists.txt for:                       pPointAssign
# Author(s):                              DerekOSullivan
#--------------------------------------------------------

SET(SRC
  PointAssign.cpp
  PointAssign_Info.cpp
  main.cpp
)

ADD_EXECUTABLE(pPointAssign ${SRC})

TARGET_LINK_LIBRARIES(pPointAssign
   ${MOOS_LIBRARIES}
   apputil
   mbutil
   m
   geometry
   color
   pthread)

