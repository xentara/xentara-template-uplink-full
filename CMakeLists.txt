cmake_minimum_required(VERSION 3.22)

# TODO: rename project and add description, URL etc.
project(xentara-template-uplink LANGUAGES CXX)

find_package(XentaraUtils REQUIRED)
find_package(XentaraPlugin REQUIRED)

add_library(
	${PROJECT_NAME} MODULE

	"src/Attributes.cpp"
	"src/Attributes.hpp"
	"src/CustomError.cpp"
	"src/CustomError.hpp"
	"src/Plugin.cpp"
	"src/Plugin.hpp"
	"src/ServiceProvider.cpp"
	"src/ServiceProvider.hpp"
	"src/TemplateClient.cpp"
	"src/TemplateClient.hpp"
	"src/TemplateRecord.cpp"
	"src/TemplateRecord.hpp"
	"src/TemplateTransaction.cpp"
	"src/TemplateTransaction.hpp"
)

target_link_libraries(
	${PROJECT_NAME}

	PRIVATE
		Xentara::xentara-utils
		Xentara::xentara-plugin
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set_target_properties(
		${PROJECT_NAME}
		
		PROPERTIES
# TODO: adjust output name to match project name
			OUTPUT_NAME XentaraTemplateUplink
			DEBUG_POSTFIX d
	)
endif()

install_xentara_plugin(${PROJECT_NAME})