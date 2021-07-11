set(PROJECT_NAME qgis_umesh)

################################################################################
# Source groups
################################################################################
set(no_group_source_files
    "include/version_number.ini"
)
source_group("" FILES ${no_group_source_files})

#set(Generated_Files
#    "generated/moc_add_current_view_window.cpp"
#    "generated/moc_edit_observation_points_window.cpp"
#    "generated/moc_map_property.cpp"
#    "generated/moc_map_property_window.cpp"
#    "generated/moc_map_time_manager_window.cpp"
#    "generated/moc_MyDrawingCanvas.cpp"
#    "generated/moc_MyEditTool.cpp"
#    "generated/moc_my_date_time_edit.cpp"
#    "generated/moc_QColorRampEditor.cpp"
#    "generated/moc_qgis_umesh.cpp"
#)
source_group("Generated Files" FILES ${Generated_Files})

set(Header_Files
    "include/AbstractCanvas.h"
    "include/AbstractCanvasListener.h"
    "include/add_current_view_window.h"
    "include/data_struct.h"
    "include/edit_observation_points_window.h"
    "include/his_cf.h"
    "include/json_reader.h"
    "include/map_property.h"
    "include/map_property_window.h"
    "include/map_time_manager_window.h"
    "include/my_date_time_edit.h"
    "include/MyDrawingCanvas.h"
    "include/MyEditTool.h"
    "include/netcdf.h"
#   "include/pattern.xpm"
    "include/perf_timer.h"
    "include/QColorRampEditor.h"
    "include/qgis_umesh.h"
#   "include/qgis_umesh_version.h"
#   "include/qgis_umesh_version.h.vcs"
#   "include/qgis_umesh_version.rc"
#   "include/qqis_umesh_version.rc"
#   "include/qqis_umesh_version.rc.vcs"
    "include/ugrid.h"
#   "include/vsi.xpm"
#   "include/vsi_disabled.xpm"
)
source_group("Header Files" FILES ${Header_Files})

set(Source_Files
    "src/add_current_view_window.cpp"
    "src/edit_observation_points_window.cpp"
    "src/his_cf.cpp"
    "src/map_property.cpp"
    "src/map_property_window.cpp"
    "src/map_time_manager_window.cpp"
    "src/my_date_time_edit.cpp"
    "src/MyDrawingCanvas.cpp"
    "src/MyEditTool.cpp"
    "src/perf_timer.cpp"
    "src/QColorRampEditor.cpp"
    "src/qgis_umesh.cpp"
    "src/qgis_umesh_version.cpp"
    "src/read_json.cpp"
    "src/ugrid.cpp"
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
    TARGET_NAME_DEBUG   "qgis_umesh"
    TARGET_NAME_RELEASE "qgis_umesh"
)
################################################################################
# Output directory
################################################################################
set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/bin/$ENV{platform}/$<CONFIG>"
    OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/$ENV{platform}/$<CONFIG>"
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
target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.;"
    "${CMAKE_CURRENT_SOURCE_DIR}/include;"
    "c:/OSGeo4W/include;"
    "c:/OSGeo4W/apps/qgis/include;"
    "$ENV{QT5DIR64OSGEO}/include;"
    "$ENV{QT5DIR64OSGEO}/include/ActiveQt;"
    "$ENV{QT5DIR64OSGEO}/include/Qt;"
    "$ENV{QT5DIR64OSGEO}/include/QtCore;"
    "$ENV{QT5DIR64OSGEO}/include/QtGui;"
    "$ENV{QT5DIR64OSGEO}/include/QtSvg;"
    "$ENV{QT5DIR64OSGEO}/include/QtWidgets;"
    "$ENV{QT5DIR64OSGEO}/include/QtXml;"
    "$ENV{BOOST_INCLUDEDIR}"
)

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
    COMMAND   $<CONFIG:Debug> call ..\\scripts\\git_insert_hash.cmd .\\include plot_cf_time_series_version.h
    COMMAND   $<CONFIG:Debug> call ..\\scripts\\git_insert_hash.cmd .\\include plot_cf_time_series_version.rc
    COMMAND $<CONFIG:Release> call ..\\scripts\\git_insert_hash.cmd .\\include plot_cf_time_series_version.h
    COMMAND $<CONFIG:Release> call ..\\scripts\\git_insert_hash.cmd .\\include plot_cf_time_series_version.rc
)

################################################################################
# Dependencies
################################################################################
set(ADDITIONAL_LIBRARY_DEPENDENCIES
    "$<$<CONFIG:Debug>:"
        "qtmaind;"
        "Qt5Cored;"
        "Qt5Guid;"
        "Qt5Widgetsd;"
        "Qt5PrintSupportd"
        "qgis_analysis"
        "qgis_core"
        "qgis_gui"
    ">"
    "$<$<CONFIG:Release>:"
        "qtmain;"
        "Qt5Core;"
        "Qt5Gui;"
        "Qt5Widgets;"
        "Qt5PrintSupport"
        "qgis_analysis"
        "qgis_core"
        "qgis_gui"
    ">"
    "netcdf"
)
target_link_libraries(${PROJECT_NAME} PRIVATE "${ADDITIONAL_LIBRARY_DEPENDENCIES}")

target_link_directories(${PROJECT_NAME} PRIVATE
    "$ENV{QT5DIR64OSGEO}/lib;"
    "c:/OSGeoW/apps/qgis/lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>"
)

