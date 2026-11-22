# zxv-link, la carga remota de ZX-V (D101): habla con la máquina por la tira
# J5 de la placa (su UART) con un adaptador USB-serie. Sin Qt. MIT, como el
# SDK. Se compila solo:
#
#   cmake -S sdk/tools/link -B build-link && cmake --build build-link
#
# o con el emulador, que lo mete en sus paquetes. Con Qt 6, también
# zxv-link-gui, con ventana. Las tramas (zxvlink.c) las compila también el
# firmware de la Pico CPU.
cmake_minimum_required(VERSION 3.16)
if(NOT CMAKE_PROJECT_NAME)
    project(zxv_link LANGUAGES C CXX)
    enable_testing()
else()
    enable_language(C)
    enable_language(CXX)                # zxv-link-gui
endif()

add_library(zxvlinkproto STATIC zxvlink.c)
target_include_directories(zxvlinkproto PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# La conversación con la máquina (el puerto serie incluido), para los dos
# programas.
add_library(zxvlinkcore STATIC linkcore.c serial.c extserve.c)
target_link_libraries(zxvlinkcore PUBLIC zxvlinkproto)

add_executable(zxv-link main.c tcp.c)
target_link_libraries(zxv-link PRIVATE zxvlinkcore)
if(WIN32)
    target_link_libraries(zxv-link PRIVATE ws2_32)
endif()
# Junto a los demás programas del emulador, donde lo cogen los paquetes.
set_target_properties(zxv-link PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# zxv-link-gui, con ventana (Qt 6): sin Qt, solo zxv-link.
if(NOT TARGET Qt6::Widgets)
    find_package(Qt6 COMPONENTS Widgets QUIET)
endif()
find_package(Qt6 COMPONENTS LinguistTools QUIET)
if(NOT TARGET Qt6::Widgets)
    message(WARNING "Qt 6 (Widgets) no encontrado: sin zxv-link-gui, la carga remota con ventana")
    return()
endif()
# Con el icono de ZX-V (gui/res: el del emulador, que no se publica) y, en
# Windows, UTF-8 como página de códigos (las rutas con tildes van por fopen).
set(_gui_res gui/res/zxv-link-gui.qrc)
if(WIN32)
    if(MSVC)
        list(APPEND _gui_res gui/res/zxv-link-gui.manifest gui/res/zxv-link-gui.rc)
    else()
        list(APPEND _gui_res gui/res/zxv-link-gui.rc gui/res/zxv-link-gui_mingw.rc)
    endif()
endif()
add_executable(zxv-link-gui WIN32 MACOSX_BUNDLE gui/main.cpp gui/window.cpp gui/window.h gui/worker.cpp gui/worker.h
               gui/icons.cpp gui/icons.h ${_gui_res})
set_target_properties(zxv-link-gui PROPERTIES AUTOMOC ON AUTORCC ON)
target_include_directories(zxv-link-gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/gui ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
target_compile_features(zxv-link-gui PRIVATE cxx_std_17)
target_link_libraries(zxv-link-gui PRIVATE zxvlinkcore Qt6::Widgets)
set_target_properties(zxv-link-gui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
# La traducción al inglés (el español es el de los textos), dentro del
# programa en :/i18n. Se pone al día con --target update_translations.
if(TARGET Qt6::lrelease)
    set(_ts_sources)
    if(Qt6_VERSION VERSION_GREATER_EQUAL 6.7)
        set(_ts_sources SOURCE_TARGETS zxv-link-gui)
    endif()
    qt_add_translations(zxv-link-gui ${_ts_sources} TS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/i18n/zxv-link-gui_en.ts
                        RESOURCE_PREFIX /i18n LUPDATE_OPTIONS -no-obsolete -locations none -source-language es)
else()
    message(WARNING "Sin Qt Linguist (LinguistTools): zxv-link-gui solo en español")
endif()
# Que la ventana se abra y se dibuje, en los dos idiomas (sin pantalla).
add_test(NAME link-ventana COMMAND zxv-link-gui -platform offscreen --lang es --screenshot link-ventana-es.png)
add_test(NAME link-ventana-en COMMAND zxv-link-gui -platform offscreen --lang en --screenshot link-ventana-en.png)
# Con un programa: el paso 3, con los ficheros de su carpeta.
add_test(NAME link-ventana-programa COMMAND zxv-link-gui -platform offscreen --lang es --screenshot
         link-ventana-programa.png ${CMAKE_CURRENT_SOURCE_DIR}/../../../examples/basic/cascade/cascade.bas)
