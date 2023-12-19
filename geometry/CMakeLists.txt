project(geometry)

set(INCLUDE_DIR "./")

file(GLOB SOURCE_FILES
./*.c
./*.c
)

add_library (${PROJECT_NAME} STATIC ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE m)
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)