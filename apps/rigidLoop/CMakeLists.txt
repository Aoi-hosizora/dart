###############################################
# apps/rigidLoop
file(GLOB rigidLoop_srcs "*.cpp")
file(GLOB rigidLoop_hdrs "*.h")
add_executable(rigidLoop ${rigidLoop_srcs} ${rigidLoop_hdrs})
target_link_libraries(rigidLoop -Wl,--push-state,--no-as-needed ${LZ4_LIBRARIES} -Wl,--pop-state)
target_link_libraries(rigidLoop dart)
set_target_properties(rigidLoop PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
