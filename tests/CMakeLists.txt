
add_executable( canim_unit_tests
	TestTextRanges.cpp
	TestHuml.cpp
)

target_link_libraries( canim_unit_tests
	Catch2::Catch2WithMain
	visuals
)

add_test( NAME unit_tests COMMAND canim_unit_tests WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" )
