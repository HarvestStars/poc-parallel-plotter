
file(GLOB PP_SOURCES "*.cpp")
file(GLOB OP_SOURCES "${PROJECT_SOURCE_DIR}/contrib/cpp-optparse/OptionParser.cpp")

set(EXEC_NAME "parallel-plotter")

add_executable(${EXEC_NAME} ${PP_SOURCES} ${OP_SOURCES})

target_link_libraries(${EXEC_NAME} shabal ${OpenCL_LIBRARY} ${Boost_FILESYSTEM_LIBRARY})

install(TARGETS ${EXEC_NAME} DESTINATION bin)
