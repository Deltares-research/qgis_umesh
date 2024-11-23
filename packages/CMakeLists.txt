message("*** Enter packages directory") 

set(CMAKE_AUTOMOC ON)
set(AUTOMOC_EXECUTABLE "$ENV{QT5DIROSGEO}/bin/moc.exe")

set(PROJECT_NAME qgis_sumesh)

################################################################################
# Source groups
################################################################################
set(no_group_source_files
    "include/version_number.ini"
)
source_group("" FILES ${no_group_source_files})

source_group("Generated Files" FILES ${Generated_Files})

set(Header_Files
    "include/AbstractCanvas.h"
    "include/AbstractCanvasListener.h"
    "include/add_current_view_window.h"
    "include/data_struct.h"
    "include/handle_vector_layers.h"
    "include/his_cf.h"
    "include/json_reader.h"
    "include/map_property.h"
    "include/map_property_window.h"
    "include/map_time_manager_window.h"
    "include/my_date_time_edit.h"
    "include/MyDrawingCanvas.h"
#    "include/MyEditTool.h"
    "include/netcdf.h"
#   "include/pattern.xpm"
    "include/perf_timer.h"
    "include/QColorRampEditor.h"
    "include/qgis_sumesh.h"
    "include/qgis_sumesh_version.h"
#   "include/qgis_umesh_version.h.vcs"
#   "include/qgis_umesh_version.rc"
#   "include/qqis_umesh_version.rc"
#   "include/qqis_umesh_version.rc.vcs"
    "include/grid.h"
#   "include/vsi.xpm"
#   "include/vsi_disabled.xpm"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "src/add_current_view_window.cpp"
    "src/handle_vector_layers.cpp"
    "src/his_cf.cpp"
    "src/map_property.cpp"
    "src/map_property_window.cpp"
    "src/map_time_manager_window.cpp"
    "src/my_date_time_edit.cpp"
    "src/MyDrawingCanvas.cpp"
#    "src/MyEditTool.cpp"
    "src/perf_timer.cpp"
    "src/QColorRampEditor.cpp"
    "src/qgis_sumesh.cpp"
    "src/qgis_sumesh_version.cpp"
    "src/read_json.cpp"
    "src/grid.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(ALL_FILES
    ${no_group_source_files}
    ${Generated_Files}
    ${Header_Files}
    ${Source_Files}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} SHARED ${ALL_FILES})

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set(ROOT_NAMESPACE interaction-example)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_GLOBAL_KEYWORD "Qt5VSv1.0"
)
################################################################################
# Target name
################################################################################
set_target_properties(${PROJECT_NAME} PROPERTIES
    TARGET_NAME_DEBUG   "qgis_sumesh"
    TARGET_NAME_RELEASE "qgis_sumesh"
)
################################################################################
# Output directory
################################################################################
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_DIRECTORY_DEBUG   "c:/OSGeo4W/apps/qgis/plugins"
    OUTPUT_DIRECTORY_RELEASE "c:/OSGeo4W/apps/qgis/plugins"
)
################################################################################
# MSVC runtime library
################################################################################
get_property(MSVC_RUNTIME_LIBRARY_DEFAULT TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY)
string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
    $<$<CONFIG:Debug>:
        MultiThreadedDebugDLL
    >
    $<$<CONFIG:Release>:
        MultiThreadedDLL
    >
    $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
)
set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

################################################################################
# Include directories
################################################################################
if (MSVC)
    message("*** MSVC 1")
    target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/.;"
        "${CMAKE_CURRENT_SOURCE_DIR}/include;"
        "c:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.40.33807/include/"
        "c:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/ucrt/"
        "c:/OSGeo4W/include;"
        "c:/OSGeo4W/apps/qgis/include;"
        "$ENV{QT5DIROSGEO}/include;"
        "$ENV{QT5DIROSGEO}/include/ActiveQt;"
        "$ENV{QT5DIROSGEO}/include/QtCore;"
        "$ENV{QT5DIROSGEO}/include/QtGui;"
        "$ENV{QT5DIROSGEO}/include/QtSvg;"
        "$ENV{QT5DIROSGEO}/include/QtWidgets;"
        "$ENV{QT5DIROSGEO}/include/QtXml;"
        "c:/boost/boost_1_85_0/"
    )
else()
    message("*** not supported, ex. UCRT64 ")
endif()

################################################################################
# Compile definitions
################################################################################
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "$<$<CONFIG:Debug>:"
        "DEBUG"
    ">"
    "$<$<CONFIG:Release>:"
        "QT_NO_DEBUG;"
        "NDEBUG"
    ">"
    "_WINDOWS;"
    "UNICODE;"
    "WIN64"
)

################################################################################
# Compile and link options
################################################################################
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /Od;
            /RTC1;
            /Zi
        >
        $<$<CONFIG:Release>:
            /O2;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT};
            -Zc:strictStrings
        >
        /W4;
        /GR;
        -w34100;
        -w34189;
        -w44996;
        ${DEFAULT_CXX_EXCEPTION_HANDLING};
        /Zc:wchar_t
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /DEBUG
        >
        $<$<CONFIG:Release>:
            /INCREMENTAL:NO
        >
        /NOLOGO;
        /SUBSYSTEM:WINDOWS;
        /NXCOMPAT;
        /DYNAMICBASE
    )
endif()

################################################################################
# Pre build events
################################################################################
add_custom_command_if(
    TARGET ${PROJECT_NAME}
    PRE_BUILD
    COMMANDS
    COMMAND   $<CONFIG:Debug> call ../scripts/git_insert_hash.cmd .\\include qgis_umesh_version.h
    COMMAND   $<CONFIG:Debug> call ../scripts/git_insert_hash.cmd .\\include qgis_umesh_version.rc
    COMMAND $<CONFIG:Release> call ../scripts/git_insert_hash.cmd .\\include qgis_umesh_version.h
    COMMAND $<CONFIG:Release> call ../scripts/git_insert_hash.cmd .\\include qgis_umesh_version.rc
)

################################################################################
# Dependencies
################################################################################
set(ADDITIONAL_LIBRARY_DEPENDENCIES
    "$<$<CONFIG:Debug>:"
        "Qt5Core"
        "Qt5Gui"
        "Qt5Widgets"
        "Qt5PrintSupport"
        "qgis_core"
        "qgis_gui"
    ">"
    "$<$<CONFIG:Release>:"
        "Qt5Core"
        "Qt5Gui"
        "Qt5Widgets"
        "Qt5PrintSupport"
        "qgis_core"
        "qgis_gui"
    ">"
    "netcdf"
)
target_link_libraries(${PROJECT_NAME} PRIVATE "${ADDITIONAL_LIBRARY_DEPENDENCIES}")

target_link_directories(${PROJECT_NAME} PRIVATE
    "$ENV{QT5DIROSGEO}/lib;"
    "c:/OSGeo4W/apps/qgis/lib"
    "c:/Program Files/netCDF 4.9.2/lib"
)

message("*** Exiting packages directory") 

