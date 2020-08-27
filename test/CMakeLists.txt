cmake_minimum_required( VERSION 3.0 )

project(test)

find_package( VTK OpenCV REQUIRED )
include( ${VTK_USE_FILE} )
include_directories( ${OpenCV_INCLUDE_DIRS} )

include_directories( inc )
add_executable( test src/main.cpp)
target_link_libraries( test dicom2meshlib ${VTK_LIBRARIES} ${OpenCV_LIBS})
target_compile_features( test PRIVATE cxx_range_for )
target_compile_options( test PRIVATE -Wall -pedantic -Wextra -Wno-extra-semi )
