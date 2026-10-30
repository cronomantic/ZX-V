# zxv-gfx, el editor de gráficos de ZX-V para el PC (licencia MIT, como el
# SDK). Se compila solo:
#
#   cmake -S sdk/tools/gfx -B build-gfx && cmake --build build-gfx
#
# o con el emulador (emulator/CMakeLists.txt lo incluye si hay Qt 6), que lo
# mete en sus paquetes. Necesita Qt 6 (Widgets); sin Qt, solo las pruebas
# del documento.
cmake_minimum_required(VERSION 3.16)
if(NOT CMAKE_PROJECT_NAME)
    project(zxv_gfx LANGUAGES CXX)
    enable_testing()
else()
    enable_language(CXX)
endif()

set(ZXV_GFX_DIR ${CMAKE_CURRENT_SOURCE_DIR})
if(MSVC)
    add_compile_options(/utf-8)          # los textos, en UTF-8 como el código
endif()
find_package(Python3 COMPONENTS Interpreter QUIET)

# El documento (sin Qt) y sus pruebas.
add_library(zxvgfxdoc STATIC doc.cpp)
target_compile_features(zxvgfxdoc PUBLIC cxx_std_17)
add_executable(zxv_gfx_doc_test tests/doc_test.cpp)
target_link_libraries(zxv_gfx_doc_test PRIVATE zxvgfxdoc)
add_test(NAME gfx-documento COMMAND zxv_gfx_doc_test)
if(Python3_FOUND)
    add_test(NAME gfx-fichero
             COMMAND ${Python3_EXECUTABLE} ${ZXV_GFX_DIR}/tests/gfx_test.py $<TARGET_FILE:zxv_gfx_doc_test>
                     ${CMAKE_CURRENT_BINARY_DIR}/gfx-fichero)
endif()

find_package(Qt6 COMPONENTS Widgets QUIET)
find_package(Qt6 COMPONENTS LinguistTools Test QUIET)
if(NOT Qt6_FOUND)
    message(WARNING "Qt 6 (Widgets) no encontrado: sin zxv-gfx, el editor de gráficos")
    return()
endif()

set(CMAKE_AUTOMOC ON)
add_executable(zxv-gfx WIN32 MACOSX_BUNDLE
    main.cpp mainwindow.cpp mainwindow.h editor.cpp editor.h canvas.cpp canvas.h panels.cpp panels.h
    png.cpp png.h importdialog.cpp importdialog.h mode7.cpp mode7.h)
set_target_properties(zxv-gfx PROPERTIES AUTOMOC ON)
target_include_directories(zxv-gfx PRIVATE ${ZXV_GFX_DIR} ${ZXV_GFX_DIR}/../../include)
target_link_libraries(zxv-gfx PRIVATE zxvgfxdoc Qt6::Widgets)
# Con el emulador, junto a zxv (donde los buscan los paquetes).
if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set_target_properties(zxv-gfx PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

# La traducción al inglés (el español es el de los textos), dentro del
# programa en :/i18n. Se pone al día con --target update_translations.
if(TARGET Qt6::lrelease)
    qt_add_translations(zxv-gfx TS_FILES ${ZXV_GFX_DIR}/i18n/zxv-gfx_en.ts RESOURCE_PREFIX /i18n
                        LUPDATE_OPTIONS -no-obsolete -locations none -source-language es)
else()
    message(WARNING "Sin Qt Linguist (LinguistTools): zxv-gfx solo en español")
endif()

# La importación de PNG, igual que png2gfx.py; y la ventana, sin pantalla.
add_executable(zxv_gfx_png_test tests/png_test.cpp png.cpp png.h)
set_target_properties(zxv_gfx_png_test PROPERTIES AUTOMOC ON)
target_include_directories(zxv_gfx_png_test PRIVATE ${ZXV_GFX_DIR})
target_link_libraries(zxv_gfx_png_test PRIVATE zxvgfxdoc Qt6::Gui)
# Los lienzos con el ratón de QTest.
if(TARGET Qt6::Test)
    add_executable(zxv_gfx_canvas_test tests/canvas_test.cpp canvas.cpp canvas.h editor.cpp editor.h png.cpp png.h)
    set_target_properties(zxv_gfx_canvas_test PROPERTIES AUTOMOC ON)
    target_include_directories(zxv_gfx_canvas_test PRIVATE ${ZXV_GFX_DIR})
    target_link_libraries(zxv_gfx_canvas_test PRIVATE zxvgfxdoc Qt6::Widgets Qt6::Test)
    add_test(NAME gfx-lienzo COMMAND zxv_gfx_canvas_test -platform offscreen)
endif()
if(Python3_FOUND)
    add_test(NAME gfx-png
             COMMAND ${Python3_EXECUTABLE} ${ZXV_GFX_DIR}/tests/png_test.py $<TARGET_FILE:zxv_gfx_png_test>
                     ${CMAKE_CURRENT_BINARY_DIR}/gfx-png)
    add_test(NAME gfx-ventana
             COMMAND ${Python3_EXECUTABLE} ${ZXV_GFX_DIR}/tests/window_test.py $<TARGET_FILE:zxv-gfx>
                     ${CMAKE_CURRENT_BINARY_DIR}/gfx-ventana)
endif()
