cmake_minimum_required( VERSION 3.0 )

project(test)

find_package( VTK REQUIRED )
include( ${VTK_USE_FILE} )

find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

include_directories( inc )
add_executable( output src/main.cpp)
target_link_libraries( output dicom2meshlib ${VTK_LIBRARIES} ${OpenCV_LIBS})
#target_link_libraries( output dicom2meshlib ${VTK_LIBRARIES})
target_compile_features( output PRIVATE cxx_range_for )
target_compile_options( output PRIVATE -Wall -pedantic -Wextra -Wno-extra-semi )
