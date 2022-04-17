cmake_minimum_required(VERSION 3.10)

project(MythonsInterpreter CXX)
set(CMAKE_CXX_STANDARD 17)

set(PROG_SRC 
    lexer.cpp
    parse.cpp
    runtime_test.cpp
    lexer_test_open.cpp
    parse_test.cpp
    statement.cpp
    main.cpp
    runtime.cpp
    statement_test.cpp )

set(PROG_INCLUDE 
    lexer.h
    parse.h
    runtime.h
    statement.h
    test_runner_p.h)
    
add_executable(MythonsInterpreter ${PROG_SRC} ${PROG_INCLUDE})

if (MSVC)
    add_compile_options(/W3 /WX)
else ()
    add_compile_options(-Werror -Wall)
endif ()

