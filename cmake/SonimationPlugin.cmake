# sonimation_add_plugin( <name> <sources>... ): a plugin of scenes, for sonimation --plugin (see core/Plugin.h).
# One of its files needs SONIMATION_PLUGIN();. It is built as <name>.dll on Windows, <name>.so elsewhere.
function( sonimation_add_plugin name )
	add_library( ${name} MODULE ${ARGN} )
	target_link_libraries( ${name} PRIVATE Sonimation::core )
	target_compile_features( ${name} PRIVATE cxx_std_20 )
	set_target_properties( ${name} PROPERTIES PREFIX "" )
	if ( MSVC )
		# The headers are UTF-8
		target_compile_options( ${name} PRIVATE /utf-8 )
	endif()
	if ( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
		# Otherwise its inline functions' statics are "unique" symbols, and it is never really unloaded, so a
		# rebuilt one could not be loaded in its place
		target_compile_options( ${name} PRIVATE -fcoroutines -fno-gnu-unique )
	elseif ( NOT MSVC )
		target_compile_options( ${name} PRIVATE -fcoroutines )
	endif()
endfunction()
