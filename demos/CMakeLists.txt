# copy needed file to build

set(DEMO_BUILD_ROOT ${PROJECT_BINARY_DIR}/demos)
    
message("${CMAKE_CURRENT_SOURCE_DIR}/triangle/shaders")
message("${DEMO_BUILD_ROOT}/triangle")



## obj2mesh
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/obj2mesh/shaders"
     DESTINATION "${DEMO_BUILD_ROOT}/obj2mesh")

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/obj2mesh/assets"
     DESTINATION "${DEMO_BUILD_ROOT}/obj2mesh/")

## shadowmap
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/shadowmap/shaders"
     DESTINATION "${DEMO_BUILD_ROOT}/shadowmap")

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/shadowmap/assets"
     DESTINATION "${DEMO_BUILD_ROOT}/shadowmap/")

## pbr
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/pbr/shaders"
     DESTINATION "${DEMO_BUILD_ROOT}/pbr")

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/pbr/assets"
     DESTINATION "${DEMO_BUILD_ROOT}/pbr/")