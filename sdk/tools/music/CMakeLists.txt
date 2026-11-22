# La música en el formato propio (.zxm, D98): la biblioteca de los
# conversores (la usan zxv-music, zxv-tracker y las pruebas) y zxv-music, la
# herramienta de línea de órdenes. Sin Qt ni SDL. MIT, como el SDK. Se
# compila solo:
#
#   cmake -S sdk/tools/music -B build-music && cmake --build build-music
#
# o con el emulador o con zxv-tracker, que la incluyen.
cmake_minimum_required(VERSION 3.16)
if(NOT CMAKE_PROJECT_NAME)
    project(zxv_music LANGUAGES C)
else()
    enable_language(C)
endif()
set(ZXV_MUSIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../music)
set(ZXV_SOUND_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../sound)   # el ADPCM (smpsynth.c)

# Sin el reproductor dentro: quien la usa lo enlaza (pt3.c, para leer los PT3).
add_library(zxvmusictools STATIC zxm_song.c zxm_pt3.c zxm_conv.c zxm_mod.c zxm_s3m.c zxm_xm.c
            ${ZXV_SOUND_DIR}/smpsynth.c)
target_include_directories(zxvmusictools PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${ZXV_MUSIC_DIR}
                           ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${ZXV_SOUND_DIR})

add_executable(zxv-music main.c ${ZXV_MUSIC_DIR}/pt3.c ${ZXV_MUSIC_DIR}/pt3tab.c)
target_link_libraries(zxv-music PRIVATE zxvmusictools)
# Junto a los demás programas del emulador, donde lo cogen los paquetes.
set_target_properties(zxv-music PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
