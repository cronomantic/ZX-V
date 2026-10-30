# zxv-tracker, el tracker de ZX-V para el PC (licencia MIT, como el SDK):
# música en módulos PT3 y efectos AYFX, que suenan con sdk/music y el AY del
# firmware. Se compila solo:
#
#   cmake -S sdk/tools/tracker -B build-tracker && cmake --build build-tracker
#
# o con el emulador (emulator/CMakeLists.txt lo incluye), que lo mete en sus
# paquetes. Necesita Qt 6 (Widgets) y SDL2 (el sonido); sin ellos, solo las
# pruebas del módulo y de los efectos.
cmake_minimum_required(VERSION 3.16)
if(NOT CMAKE_PROJECT_NAME)
    project(zxv_tracker LANGUAGES C CXX)
    enable_testing()
else()
    enable_language(C)
    enable_language(CXX)
endif()

set(ZXV_TRACKER_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ZXV_SDK_DIR ${ZXV_TRACKER_DIR}/../..)
if(MSVC)
    add_compile_options(/utf-8)          # los textos, en UTF-8 como el código
endif()
find_package(Python3 COMPONENTS Interpreter QUIET)

# El módulo, los efectos y lo que suena (sin Qt): sdk/music, el mismo
# código que la máquina, y el AY de las Pico.
set(MUSIC_DIR ${ZXV_SDK_DIR}/music)
add_library(zxvtrkcore STATIC song.cpp sfx.cpp engine.cpp
    ${MUSIC_DIR}/core.c ${MUSIC_DIR}/pt3.c ${MUSIC_DIR}/pt3tab.c ${MUSIC_DIR}/psg.c ${MUSIC_DIR}/pt2.c
    ${MUSIC_DIR}/stc.c ${MUSIC_DIR}/asc.c ${MUSIC_DIR}/afx.c ${ZXV_SDK_DIR}/../pico/common/aysynth.c)
target_include_directories(zxvtrkcore PUBLIC ${MUSIC_DIR} ${ZXV_SDK_DIR}/include ${ZXV_SDK_DIR}/../pico/common)
target_compile_features(zxvtrkcore PUBLIC cxx_std_17)

add_executable(zxv_tracker_song_test tests/song_test.cpp)
target_link_libraries(zxv_tracker_song_test PRIVATE zxvtrkcore)
add_test(NAME tracker-modulo COMMAND zxv_tracker_song_test)
add_executable(zxv_tracker_sfx_test tests/sfx_test.cpp)
target_link_libraries(zxv_tracker_sfx_test PRIVATE zxvtrkcore)
add_test(NAME tracker-efectos COMMAND zxv_tracker_sfx_test)
if(Python3_FOUND)
    # Los módulos de prueba de sdk/music (los mismos de musica-formatos):
    # se leen y se guardan; los que no tienen órdenes raras suenan igual.
    add_test(NAME tracker-pt3
             COMMAND ${Python3_EXECUTABLE} ${ZXV_TRACKER_DIR}/tests/pt3_test.py $<TARGET_FILE:zxv_tracker_song_test>
                     ${ZXV_SDK_DIR}/../emulator/tests/music/mkmodules.py ${CMAKE_CURRENT_BINARY_DIR}/tracker-pt3)
endif()

find_package(Qt6 COMPONENTS Widgets QUIET)
find_package(Qt6 COMPONENTS LinguistTools Test QUIET)
if(NOT TARGET SDL2::SDL2)
    find_package(SDL2 CONFIG QUIET)
endif()
if(NOT Qt6_FOUND OR NOT TARGET SDL2::SDL2)
    message(WARNING "Qt 6 (Widgets) o SDL2 no encontrados: sin zxv-tracker")
    return()
endif()

set(CMAKE_AUTOMOC ON)
add_executable(zxv-tracker WIN32 MACOSX_BUNDLE
    main.cpp mainwindow.cpp mainwindow.h editor.cpp editor.h patternview.cpp patternview.h
    lines.cpp lines.h piano.cpp piano.h audio.cpp audio.h)
set_target_properties(zxv-tracker PROPERTIES AUTOMOC ON)
target_include_directories(zxv-tracker PRIVATE ${ZXV_TRACKER_DIR})
# main() es el de Qt (en Windows, Qt pone su WinMain): sin SDL2main.
target_link_libraries(zxv-tracker PRIVATE zxvtrkcore Qt6::Widgets SDL2::SDL2)
# Con el emulador, junto a zxv (donde los buscan los paquetes).
if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set_target_properties(zxv-tracker PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

# La traducción al inglés (el español es el de los textos), dentro del
# programa en :/i18n. Se pone al día con --target update_translations.
if(TARGET Qt6::lrelease)
    qt_add_translations(zxv-tracker TS_FILES ${ZXV_TRACKER_DIR}/i18n/zxv-tracker_en.ts RESOURCE_PREFIX /i18n
                        LUPDATE_OPTIONS -no-obsolete -locations none -source-language es)
else()
    message(WARNING "Sin Qt Linguist (LinguistTools): zxv-tracker solo en español")
endif()

if(TARGET Qt6::Test)
    add_executable(zxv_tracker_view_test tests/view_test.cpp patternview.cpp patternview.h lines.cpp lines.h
                   editor.cpp editor.h piano.cpp piano.h)
    set_target_properties(zxv_tracker_view_test PROPERTIES AUTOMOC ON)
    target_include_directories(zxv_tracker_view_test PRIVATE ${ZXV_TRACKER_DIR})
    target_link_libraries(zxv_tracker_view_test PRIVATE zxvtrkcore Qt6::Widgets Qt6::Test)
    add_test(NAME tracker-editor COMMAND zxv_tracker_view_test -platform offscreen)
endif()
if(Python3_FOUND)
    add_test(NAME tracker-ventana
             COMMAND ${Python3_EXECUTABLE} ${ZXV_TRACKER_DIR}/tests/window_test.py $<TARGET_FILE:zxv-tracker>
                     ${CMAKE_CURRENT_BINARY_DIR}/tracker-ventana)
endif()
