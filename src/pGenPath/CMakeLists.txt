#--------------------------------------------------------
# The CMakeLists.txt for:                       pGenPath
# Author(s):                              DerekOSullivan
#--------------------------------------------------------

SET(SRC
  GenPath.cpp
  GenPath_Info.cpp
  main.cpp
)

ADD_EXECUTABLE(pGenPath ${SRC})

if(APPLE)
    set(MBUTIL_LINK mbutil)
else()
    set(MBUTIL_LINK -Wl, --whole-archive mbutil -Wl, --no-whole-archive)
endif()


TARGET_LINK_LIBRARIES(pGenPath
   ${MOOS_LIBRARIES}
   apputil
   ${MBUTIL_LINK}
   m
   geometry
   pthread)
