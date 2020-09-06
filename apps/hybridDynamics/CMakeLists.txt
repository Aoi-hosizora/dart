###############################################
# apps/hybridDynamics
file(GLOB hybridDynamics_srcs "*.cpp")
file(GLOB hybridDynamics_hdrs "*.h")
add_executable(hybridDynamics ${hybridDynamics_srcs} ${hybridDynamics_hdrs})
target_link_libraries(hybridDynamics -Wl,--push-state,--no-as-needed ${LZ4_LIBRARIES} -Wl,--pop-state)
target_link_libraries(hybridDynamics dart)
set_target_properties(hybridDynamics PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
