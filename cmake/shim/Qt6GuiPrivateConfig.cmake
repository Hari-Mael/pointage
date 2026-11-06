if(NOT TARGET Qt6::GuiPrivate)
    get_filename_component(_pointage_qt_prefix "${Qt6Core_DIR}/../../.." ABSOLUTE)
    if(DEFINED Qt6_VERSION)
        set(_pointage_qt_version "${Qt6_VERSION}")
    else()
        set(_pointage_qt_version "${Qt6Gui_VERSION}")
    endif()
    add_library(Qt6::GuiPrivate INTERFACE IMPORTED)
    set_target_properties(Qt6::GuiPrivate PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES
            "${_pointage_qt_prefix}/include/QtGui/${_pointage_qt_version}/QtGui"
        INTERFACE_LINK_LIBRARIES Qt6::Gui)
endif()
set(Qt6GuiPrivate_FOUND TRUE)
