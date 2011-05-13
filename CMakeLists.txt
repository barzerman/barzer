cmake_minimum_required (VERSION 2.6)
project (barzer)
set (barzer_VERSION_MAJOR 0)
set (barzer_VERSION_MINOR 1)

set(EN_DIR "lg_en")
set(RU_DIR "lg_ru")


find_package(Boost COMPONENTS system REQUIRED)
link_directories ( ${boost_LIBRARY_DIRS} )

include_directories("${PROJECT_BINARY_DIR}"
                    "${PROJECT_SOURCE_DIR}"
                    "${PROJECT_SOURCE_DIR}/ay"
                    "${PROJECT_SOURCE_DIR}/${EN_DIR}"
                    "${PROJECT_SOURCE_DIR}/${RU_DIR}"
                    ${boost_INCLUDE_DIRS})
add_subdirectory (ay)

set(EXTRA_LIBS ${EXTRA_LIBS} ${Boost_SYSTEM_LIBRARY} ay expat)

set(SOURCES barzer_barz.cpp
            barzer_el_wildcard.cpp
            barzer_basic_types.cpp
            barzer_el_xml.cpp
            barzer_language.cpp
            barzer_date_util.cpp
            barzer_lexer.cpp
            barzer_dtaindex.cpp
            barzer_loader_xml.cpp
            barzer_el_btnd.cpp
            barzer_parse.cpp
            barzer_el_chain.cpp
            barzer_parse_types.cpp
            barzer_el_function.cpp 
            barzer_server.cpp
            barzer_el_matcher.cpp
            barzer_server_request.cpp
            barzer_el_parser.cpp
            barzer_server_response.cpp
            barzer_el_rewriter.cpp
            barzer_shell.cpp
            barzer_el_trie.cpp
            barzer_storage_types.cpp
            barzer_el_trie_processor.cpp
            barzer_token.cpp
            barzer_el_trie_shell.cpp 
            barzer_universe.cpp
            barzer_el_trie_walker.cpp
            barzer_dict.cpp
            barzer_settings.cpp
            "${EN_DIR}/barzer_en_date_util.cpp"
            "${EN_DIR}/barzer_en_lex.cpp"
            "${RU_DIR}/barzer_ru_date_util.cpp"
            "${RU_DIR}/barzer_ru_lex.cpp"
            )

IF( ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" )
    list(APPEND CMAKE_CXX_FLAGS "-Wnon-virtual-dtor -Wall -g")
ENDIF( ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" )

IF( ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU" )
    list(APPEND CMAKE_CXX_FLAGS "-Wnon-virtual-dtor -Wall -g")
ENDIF( ${CMAKE_CXX_COMPILER_ID} MATCHES "GNU" )

add_executable(barzer.exe barzer.cpp ${SOURCES})
target_link_libraries (barzer.exe  ${EXTRA_LIBS})
