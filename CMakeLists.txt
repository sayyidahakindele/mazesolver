cmake_minimum_required(VERSION 3.21)

project(MazeSolver LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(MazeSolver
    main.cpp
    MainWindow.h
    MainWindow.cpp
    MazeGenerator.h
    MazeGenerator.cpp
    MazeSolver.h
    MazeSolver.cpp
    MazeWidget.h
    MazeWidget.cpp
    Player.h
    Player.cpp
)

target_link_libraries(MazeSolver PRIVATE Qt6::Widgets)

# Windows GUI app: no console window pops up when you double-click the .exe.
set_target_properties(MazeSolver PROPERTIES WIN32_EXECUTABLE TRUE)