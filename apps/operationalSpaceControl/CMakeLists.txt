###############################################
# apps/operationalSpaceControl
file(GLOB operationalSpaceControl_srcs "*.cpp")
file(GLOB operationalSpaceControl_hdrs "*.h")
add_executable(operationalSpaceControl ${operationalSpaceControl_srcs} ${operationalSpaceControl_hdrs})
target_link_libraries(operationalSpaceControl -Wl,--push-state,--no-as-needed ${LZ4_LIBRARIES} -Wl,--pop-state)
target_link_libraries(operationalSpaceControl dart)
set_target_properties(operationalSpaceControl PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
