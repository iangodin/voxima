
add_executable( sonimation_unit_tests
	TestTextRanges.cpp
	TestHuml.cpp
	TestIntervalMap.cpp
	TestStyledText.cpp
	TestCreateStyledText.cpp
	TestRadical.cpp
	TestFraction.cpp
	TestLayout.cpp
	TestMathStyle.cpp
	TestScripts.cpp
	TestMathAlphabet.cpp
	TestRow.cpp
	TestSVGWriter.cpp
	TestLimits.cpp
	TestDelimited.cpp
	TestAccents.cpp
	TestMatrix.cpp
)

target_link_libraries( sonimation_unit_tests
	Catch2::Catch2WithMain
	visuals
)

add_test( NAME unit_tests COMMAND sonimation_unit_tests WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" )
