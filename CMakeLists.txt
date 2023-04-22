file(GLOB *.cpp *.h FILES)

add_library(Write write.cpp write.h)
target_link_libraries(Write pico_stdlib)