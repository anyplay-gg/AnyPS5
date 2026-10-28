add_library(libScePad SHARED EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_SOURCE_DIR}/Export.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PadState.cpp
)
target_include_directories(libScePad PRIVATE ${LIBS_INCLUDE_DIR})
target_link_libraries(libScePad PRIVATE libSceNetCtl)
set_target_properties(libScePad PROPERTIES CXX_EXTENSIONS OFF)
configure_windows_unwind(libScePad)

if(BUILD_TESTING)
    add_test_executable(guest_pad_tests ${LIBS_INCLUDE_DIR}/tests/GuestPad.cpp)
    target_include_directories(guest_pad_tests PRIVATE ${LIBS_INCLUDE_DIR} ${ANYPS5_SDL2_INCLUDE_DIR})
    target_link_libraries(guest_pad_tests PRIVATE libScePad libc)
    configure_windows_unwind(guest_pad_tests)
    add_test(NAME guest_pad COMMAND guest_pad_tests)
    if(WIN32)
        set_tests_properties(guest_pad PROPERTIES ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:libScePad>")
    endif()
endif()

if(BUILD_TESTING)
    add_test_executable(guest_pad_vibration_mode_tests ${LIBS_INCLUDE_DIR}/tests/GuestPadVibrationMode.cpp)
    target_include_directories(guest_pad_vibration_mode_tests PRIVATE ${LIBS_INCLUDE_DIR})
    target_link_libraries(guest_pad_vibration_mode_tests PRIVATE libScePad libc)
    configure_windows_unwind(guest_pad_vibration_mode_tests)
    add_test(NAME guest_pad_vibration_mode COMMAND guest_pad_vibration_mode_tests)
    if(WIN32)
        set_tests_properties(guest_pad_vibration_mode PROPERTIES ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:libScePad>")
    endif()
endif()

if(BUILD_TESTING)
    add_test_executable(pad_share_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/SharePad.cpp)
    target_include_directories(pad_share_tests PRIVATE ${LIBS_INCLUDE_DIR})
    target_link_libraries(pad_share_tests PRIVATE libScePad libSceNetCtl libkernel libc)
    configure_windows_unwind(pad_share_tests)
    add_test(NAME pad_share COMMAND pad_share_tests)
    set_tests_properties(pad_share PROPERTIES TIMEOUT 40)
    if(WIN32)
        target_link_libraries(pad_share_tests PRIVATE ws2_32)
        set_tests_properties(pad_share PROPERTIES ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:libScePad>")
    endif()

    add_test_executable(pad_share_chord_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/ShareOverrideChord.cpp)
    target_include_directories(pad_share_chord_tests PRIVATE ${LIBS_INCLUDE_DIR})
    configure_windows_unwind(pad_share_chord_tests)
    add_test(NAME pad_share_chord COMMAND pad_share_chord_tests)
endif()
