###############################################
# apps/addDeleteSkels
file(GLOB addDeleteSkels_srcs "*.cpp")
file(GLOB addDeleteSkels_hdrs "*.h")
add_executable(addDeleteSkels ${addDeleteSkels_srcs} ${addDeleteSkels_hdrs})
target_link_libraries(addDeleteSkels -Wl,--push-state,--no-as-needed ${LZ4_LIBRARIES} -Wl,--pop-state)
target_link_libraries(addDeleteSkels dart)
set_target_properties(addDeleteSkels PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
