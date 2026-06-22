# Действия, необходимые для удаления установлененных файлов

if (NOT EXISTS "${CMAKE_BINARY_DIR}/install_manifest.txt")
    message(FATAL_ERROR "Failed to uninstall FILES_TO_UNINSTALL! Reason: install_manifest.txt not found")
endif()

file(READ "${CMAKE_BINARY_DIR}/install_manifest.txt" FILES_TO_UNINSTALL)
string(REPLACE "\n" ";" FILES_TO_UNINSTALL "${FILES_TO_UNINSTALL}")

foreach(FILE_TO_UNINSTALL ${FILES_TO_UNINSTALL})
    if(EXISTS "${FILE_TO_UNINSTALL}")
        message(STATUS "Removing ${FILE_TO_UNINSTALL}")
        file(REMOVE "${FILE_TO_UNINSTALL}")
    endif()
endforeach()

