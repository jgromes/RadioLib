# this is required for ESP IDF
idf_component_register()

# if you did not build RadioLib as shared library (see wiki),
# you will have to add it as source directory
# the following is just an example, yours will likely be different
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../RadioLib" "${CMAKE_CURRENT_BINARY_DIR}/RadioLib")

# link RadioLib as interface
target_link_libraries(${COMPONENT_LIB} INTERFACE RadioLib)
