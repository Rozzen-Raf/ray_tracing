project(thread_pool)

set(INCLUDE_DIR "./include")

file(GLOB SOURCE_FILES
./src/*.c
)

file(GLOB HEADER_FILES 
./include/*.h
)

add_library (${PROJECT_NAME} STATIC ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDE_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)
