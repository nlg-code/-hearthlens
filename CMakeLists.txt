cmake_minimum_required(VERSION 3.16)
project(HearthLens LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

add_executable(HearthLens
    src/main.cpp
    src/LogWatcher.cpp
    src/LogWatcher.h
    src/PowerLogParser.cpp
    src/SecretsDatabase.h
    src/PowerLogParser.h
    src/DecksLogParser.cpp
    src/DecksLogParser.h
    src/ImageCache.cpp
    src/ImageCache.h
    src/CardDelegate.cpp
    src/CardDelegate.h
    src/GameState.cpp
    src/GameState.h
    src/CardDatabase.cpp
    src/CardDatabase.h
    src/OverlayWindow.cpp
    src/OverlayWindow.h
    src/CardListPanel.cpp
    src/CardListPanel.h
    src/CardTooltip.cpp
    src/CardTooltip.h
    src/CardDetailTooltip.cpp
    src/CardDetailTooltip.h
    src/AnimalCompanionWidget.cpp
    src/AnimalCompanionWidget.h
    src/SessionManager.cpp
    src/SessionManager.h
    resources/resources.qrc
)

target_link_libraries(HearthLens PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

if(WIN32)
    set_target_properties(HearthLens PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
