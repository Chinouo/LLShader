# copy needed file to build

set(DEMO_BUILD_ROOT ${PROJECT_BINARY_DIR}/demos)
    

## triangle
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/triangle/shaders"
     DESTINATION "${DEMO_BUILD_ROOT}/triangle")

message("${CMAKE_CURRENT_SOURCE_DIR}/triangle/shaders")
message("${DEMO_BUILD_ROOT}/triangle")

## pcss
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/pcss/shaders"
     DESTINATION "${DEMO_BUILD_ROOT}/pcss")

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/pcss/assets"
     DESTINATION "${DEMO_BUILD_ROOT}/pcss/")

## shadow
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/shadow/shaders"
     DESTINATION "${DEMO_BUILD_ROOT}/shadow")

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/shadow/assets"
     DESTINATION "${DEMO_BUILD_ROOT}/shadow/")

## obj2mesh
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/obj2mesh/shaders"
     DESTINATION "${DEMO_BUILD_ROOT}/obj2mesh")

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/obj2mesh/assets"
     DESTINATION "${DEMO_BUILD_ROOT}/obj2mesh/")