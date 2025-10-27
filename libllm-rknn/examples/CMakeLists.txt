# Example applications

add_executable(llm_demo llm_demo.cpp)

target_link_libraries(llm_demo
    PRIVATE
        llm-rknn
)

# Install example
install(TARGETS llm_demo
    RUNTIME DESTINATION bin
)

# Create a simple test example
add_executable(llm_simple_test simple_test.cpp)

target_link_libraries(llm_simple_test
    PRIVATE
        llm-rknn
)

install(TARGETS llm_simple_test
    RUNTIME DESTINATION bin
)
