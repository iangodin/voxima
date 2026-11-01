
add_executable( voxima_unit_tests
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
	TestExpr.cpp
	TestTags.cpp
	TestSilence.cpp
	TestWait.cpp
	TestEase.cpp
	TestSound.cpp
	TestHighlight.cpp
	TestCreateCode.cpp
	TestTyping.cpp
	TestLineHighlight.cpp
	TestPlot.cpp
	TestCamera.cpp
	TestPlacement.cpp
	TestUpdaters.cpp
	TestConnector.cpp
	TestCallout.cpp
	TestBrace.cpp
	TestDrawAxes.cpp
	TestLegend.cpp
	TestBarChart.cpp
	TestScatter.cpp
	TestOperators.cpp
	TestSymbols.cpp
	TestGeometry.cpp
	TestNumberLine.cpp
	TestDiagram.cpp
	TestAnimate.cpp
	TestTrim.cpp
	TestPanelListTable.cpp
	TestPalette.cpp
	TestFontStacks.cpp
	TestThemeFile.cpp
	TestVolume.cpp
	TestLanguage.cpp
	TestScriptLanguageFallback.cpp
	TestTimedWord.cpp
	TestMarkers.cpp
	TestAltText.cpp
	TestTimeline.cpp
	TestRetime.cpp
	TestCompress.cpp
	TestWords.cpp
	TestOutline.cpp
	TestMorph.cpp
	TestMorphLeaves.cpp
	TestSceneRegistry.cpp
	TestWordEffects.cpp
	TestMissingGlyph.cpp
	TestPlugin.cpp
)

target_link_libraries( voxima_unit_tests
	Catch2::Catch2WithMain
	visuals
)

target_compile_definitions( voxima_unit_tests PRIVATE TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data" )

# Plugins for TestPlugin.cpp to load
voxima_add_plugin( test_plugin plugin/TestPluginScenes.cpp )
voxima_add_plugin( test_plugin_wrong_abi plugin/TestPluginWrongAbi.cpp )
add_dependencies( voxima_unit_tests test_plugin test_plugin_wrong_abi )
target_compile_definitions( voxima_unit_tests PRIVATE
	VOXIMA_TEST_PLUGIN="$<TARGET_FILE:test_plugin>"
	VOXIMA_TEST_PLUGIN_WRONG_ABI="$<TARGET_FILE:test_plugin_wrong_abi>" )

# Most tests set text in the test themes (tests/data/themes), which need only the fonts of download_fonts.py --tests.
# The ones of the built-in themes themselves need all the fonts, and are a test of their own (label all-fonts), so
# CI can leave them out: ctest -LE all-fonts.
add_test( NAME unit_tests COMMAND voxima_unit_tests "~[all-fonts]" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" )
add_test( NAME unit_tests_all_fonts COMMAND voxima_unit_tests "[all-fonts]" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" )
set_tests_properties( unit_tests_all_fonts PROPERTIES LABELS all-fonts )

copy_runtime_dlls( voxima_unit_tests )
