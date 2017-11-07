cmake_minimum_required (VERSION 2.6)
project (NMEAlib)

# modules loading
set(CMAKE_MODULE_PATH
  ${CMAKE_SOURCE_DIR}/cmake/Modules
)

include(DefineCMakeDefaults)
include(DefinePlatformDefaults)
include(DefineCompilerFlags)
include(DefineInstallationPaths)

# library setup
set(LIBRARY_VERSION "0.1.1")
set(LIBRARY_SOVERSION "0")

include_directories("include")

add_library(${PROJECT_NAME} SHARED src/context.c src/generate.c src/generator.c src/gmath.c src/info.c src/parse.c src/parser.c src/sentence.c src/time.c src/tok.c)

set_target_properties(
    ${PROJECT_NAME}
	PROPERTIES
		OUTPUT_NAME
			nmea
		VERSION
			${LIBRARY_VERSION}
		SOVERSION
			${LIBRARY_SOVERSION}
		DEFINE_SYMBOL
			NMEA_EXPORTS
)


install(TARGETS ${PROJECT_NAME} 
		LIBRARY DESTINATION ${LIB_INSTALL_DIR}
		COMPONENT libraries)

# headers setup
set(includedir include)
install(
  FILES
	${includedir}/nmea/config.h
	${includedir}/nmea/context.h
	${includedir}/nmea/generate.h
	${includedir}/nmea/generator.h
	${includedir}/nmea/gmath.h
	${includedir}/nmea/info.h
	${includedir}/nmea/nmea.h
	${includedir}/nmea/parse.h
	${includedir}/nmea/parser.h
	${includedir}/nmea/sentence.h
	${includedir}/nmea/time.h
	${includedir}/nmea/tok.h
	${includedir}/nmea/units.h
  DESTINATION
    ${INCLUDE_INSTALL_DIR}/nmea/
  COMPONENT
    headers
)

