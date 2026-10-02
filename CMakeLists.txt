cmake_minimum_required(VERSION 3.16)

project(CalculatorThatStaysOnline LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(CalculatorThatStaysOnline
    httpresponse.h httpresponse.cpp
    httpparser.h httpparser.cpp
    server.h server.cpp
main.cpp)

include(GNUInstallDirs)
install(TARGETS CalculatorThatStaysOnline
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
