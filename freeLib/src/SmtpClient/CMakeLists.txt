# -DSmtpMime_EXAMPLES=[true|false]
#  Build the examples.
#  Default=true

if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

project(SmtpMime VERSION 2.0.0 LANGUAGES CXX)

cmake_minimum_required(VERSION 3.16.0)
SET(CMAKE_COLOR_MAKEFILE ON)
#SET(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

option(SMTPMIME_EXAMPES "Build examples" ON)
option(SMTPMIME_STATIC "Build library static" OFF)

find_package( Qt5 REQUIRED COMPONENTS Core Network OPTIONAL_COMPONENTS Widgets)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")

set(HEADERS
    src/emailaddress.h
    src/mimeattachment.h
    src/mimefile.h
    src/mimehtml.h
    src/mimeinlinefile.h
    src/mimemessage.h
    src/mimepart.h
    src/mimetext.h
    src/smtpclient.h
    src/SmtpMime
    src/quotedprintable.h
    src/mimemultipart.h
    src/mimecontentformatter.h
    src/smtpmime_global.h
)

set(SOURCES
    src/emailaddress.cpp
    src/mimeattachment.cpp
    src/mimefile.cpp
    src/mimehtml.cpp
    src/mimeinlinefile.cpp
    src/mimemessage.cpp
    src/mimepart.cpp
    src/mimetext.cpp
    src/smtpclient.cpp
    src/quotedprintable.cpp
    src/mimemultipart.cpp
    src/mimecontentencoder.cpp
    src/mimebase64encoder.cpp
    src/mimeqpencoder.cpp
    src/mimeqpformatter.cpp
    src/mimebase64formatter.cpp
    src/mimecontentformatter.cpp
)

if (NOT SMTPMIME_STATIC)
    add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})

    target_compile_definitions(${PROJECT_NAME}
	    PRIVATE SMTP_MIME_LIBRARY
    )
else()
    add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

    target_compile_definitions(${PROJECT_NAME}
        PUBLIC SMTP_MIME_LIBRARY_STATIC
    )
endif()

target_link_libraries(${PROJECT_NAME}
     PUBLIC Qt5::Core Qt5::Network
)
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${${PROJECT_NAME}_VERSION})
target_include_directories(${PROJECT_NAME} PUBLIC src)

if(WIN32)
    set(CMAKE_DEBUG_POSTFIX "d")
    set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
endif()

include(CMakePackageConfigHelpers)
write_basic_package_version_file( ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)

if(SMTPMIME_EXAMPLES)
    add_subdirectory(demos)
endif()