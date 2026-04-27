
add_executable( poc main.cpp )

target_compile_features( poc PUBLIC cxx_std_20 )
target_compile_options( poc PUBLIC -fcoroutines )

