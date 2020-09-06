###############################################
# apps/atlasSimbicon
file(GLOB atlasSimbicon_srcs "*.cpp")
file(GLOB atlasSimbicon_hdrs "*.h")
add_executable(atlasSimbicon ${atlasSimbicon_srcs} ${atlasSimbicon_hdrs})
target_link_libraries(atlasSimbicon -Wl,--push-state,--no-as-needed ${LZ4_LIBRARIES} -Wl,--pop-state)
target_link_libraries(atlasSimbicon dart)
set_target_properties(atlasSimbicon PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
