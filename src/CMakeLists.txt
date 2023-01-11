# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# Name:        tests/CMakeLists.txt
# Project:     cdBuild: Common Data, Build Database files.
# Author:      Nick Matthews
# Website:     http://thefamilypack.org
# Created:     9th January 2023
# Copyright:   Copyright (c) 2023, Nick Matthews.
# Licence:     GNU GPLv3
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

set( TFP_FILL_SRC_FILES
    fiCommon.cpp
    fiMedia.cpp
    fiRefMarkup.cpp
    nkMain.cpp
    nkRecHelpers.cpp
    nkRefDocCustom.cpp
    nkRefDocuments.cpp
    nkXmlHelpers.cpp
    xml2.cpp
)

set( TFP_FILL_SRC_HEADERS
    fiCommon.h
    fiRefMarkup.h
    nkMain.h
    xml2.h
)

include_directories( ./ )
include_directories( ../../tfp/include )

add_executable( fill ${TFP_FILL_SRC_FILES} ${TFP_FILL_SRC_HEADERS} )

target_link_libraries( fill PRIVATE reccl wx::expat )
