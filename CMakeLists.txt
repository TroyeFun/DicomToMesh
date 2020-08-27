cmake_minimum_required( VERSION 3.0 )

project( d2m )

add_subdirectory(lib)
add_subdirectory(dicom2mesh)
add_subdirectory(test)

option(BUILD_GUI "Build Dicom2Mesh GUI" OFF)
if(BUILD_GUI)
    add_subdirectory(gui)
endif(BUILD_GUI)


