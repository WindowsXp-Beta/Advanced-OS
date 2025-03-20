# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

project(project3)

file(GLOB SRCS *.cc *.h)

add_library(vendor_client vendor_client.cc)
target_link_libraries(vendor_client
        gRPC::grpc++
        p3protolib
)
add_dependencies(vendor_client p3protolib)
target_compile_definitions(vendor_client
        PRIVATE
        $<$<CONFIG:Debug>:DEBUG>
)

# add_executable(store ${SRCS})
add_executable(store store.cc)
target_link_libraries(store
	Threads::Threads
       	gRPC::grpc++
	p3protolib
        vendor_client)
add_dependencies(store p3protolib)
target_compile_definitions(store
        PRIVATE
        $<$<CONFIG:Debug>:DEBUG>
)
set_target_properties(store PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)


target_link_libraries(store vendor_client)

#vendor_addresses.txt file symlink rule
set (source "${CMAKE_CURRENT_SOURCE_DIR}/vendor_addresses.txt")
set (destination "${CMAKE_BINARY_DIR}/bin/vendor_addresses.txt")
add_custom_command(
        TARGET store POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${source} ${destination}
        DEPENDS ${destination}
        COMMENT "symbolic link ${destination} created."
)
