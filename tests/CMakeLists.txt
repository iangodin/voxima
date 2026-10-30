
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

target_link_libraries( sonimation_unit_tests
	Catch2::Catch2WithMain
	visuals
)

target_compile_definitions( sonimation_unit_tests PRIVATE SONIMATION_TEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/data" )

# Plugins for TestPlugin.cpp to load
sonimation_add_plugin( test_plugin plugin/TestPluginScenes.cpp )
sonimation_add_plugin( test_plugin_wrong_abi plugin/TestPluginWrongAbi.cpp )
add_dependencies( sonimation_unit_tests test_plugin test_plugin_wrong_abi )
target_compile_definitions( sonimation_unit_tests PRIVATE
	SONIMATION_TEST_PLUGIN="$<TARGET_FILE:test_plugin>"
	SONIMATION_TEST_PLUGIN_WRONG_ABI="$<TARGET_FILE:test_plugin_wrong_abi>" )

add_test( NAME unit_tests COMMAND sonimation_unit_tests WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" )

copy_runtime_dlls( sonimation_unit_tests )
