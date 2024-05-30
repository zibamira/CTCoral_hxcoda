#######################################
# Include necessary AvizoApps utility files
#######################################
include(CopyUtils)

#######################################
# Search all required packages
#######################################
find_package(AvizoAppsQt5 REQUIRED)
find_package(AvizoAppsOIV REQUIRED)

if(NOT WIN32)
    find_package(AvizoAppsFFTW3 REQUIRED) # prepack only used on Linux
endif()

#######################################
# Create a target for hxcoda
#######################################
avizoapps_add_library(hxcoda SHARED
    CXX_SOURCES
        internal/Coda.h
        internal/Coda.cpp
        internal/CodaProcess.h
        internal/CodaProcess.cpp
        internal/PortCoda.h
        internal/PortCoda.cpp
        HxCodaVertex.h
        HxCodaVertex.cpp
        HxCodaVertexColormap.h
        HxCodaVertexColormap.cpp
        HxCodaEdge.h
        HxCodaEdge.cpp
        HxCodaEdgeColormap.h
        HxCodaEdgeColormap.cpp
        HxCodaGraph.h
        HxCodaGraph.cpp
        HxCodaVertexFilter.h
        HxCodaVertexFilter.cpp
        HxCodaVertexSelection.h
        HxCodaVertexSelection.cpp
        HxCodaEdgeFilter.h
        HxCodaEdgeFilter.cpp
    LABELS
        Common
)

target_link_libraries(hxcoda
    PUBLIC
        AvizoApps::Inventor
        AvizoApps::InventorBase
        AvizoApps::Qt5Core
        hxcore
        hxfield
        hxspreadsheet
        hxspatialgraph
        hxspatialgraphspreadsheet
        hxquant2
        hxquant2custom
        hxvolumeviz2
        hxlineviewer
)

#######################################
# Add files to copy to the same location within the build folder
#######################################
avizoapps_target_share(hxcoda
    SHARE
        share/resources/hxcoda.rc
        share/python_script_objects/CodaLoadNumpy.pyscro
        share/python_script_objects/CodaSaveNumpy.pyscro
)