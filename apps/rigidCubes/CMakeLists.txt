###############################################
# apps/rigidCubes
file(GLOB rigidCubes_srcs "*.cpp")
file(GLOB rigidCubes_hdrs "*.h")
add_executable(rigidCubes ${rigidCubes_srcs} ${rigidCubes_hdrs})
target_link_libraries(rigidCubes -Wl,--push-state,--no-as-needed ${LZ4_LIBRARIES} -Wl,--pop-state)
target_link_libraries(rigidCubes dart)
set_target_properties(rigidCubes PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
