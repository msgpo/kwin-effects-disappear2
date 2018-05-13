# - Try to find kwineffects
#
# Once done this will define
#  KWINEFFECTS_FOUND - System has kwineffects
#  KWINEFFECTS_INCLUDE_DIRS - The kwineffects include directories
#  KWINEFFECTS_LIBRARIES - The libraries needed to use kwineffects
#  KWINEFFECTS_GLUTILS_INCLUDE_DIRS - The kwinglutils include directories
#  KWINEFFECTS_GLUTILS_LIBRARIES - The libraries needed to use kwinglutils
#
# Also, once done this will define the following targets
#  KWinEffects::KWinEffects
#  KWinEffects::GLUtils
#

find_path (KWINEFFECTS_INCLUDE_DIRS
    NAMES kwineffects.h
)

find_path (KWINEFFECTS_GLUTILS_INCLUDE_DIRS
    NAMES kwinglutils.h
)

find_library (KWINEFFECTS_LIBRARY
    NAMES kwineffects
)

find_library (KWINEFFECTS_GLUTILS_LIBRARY
    NAMES kwinglutils
)

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (KWinEffects DEFAULT_MSG
    KWINEFFECTS_LIBRARY KWINEFFECTS_INCLUDE_DIRS
    KWINEFFECTS_GLUTILS_LIBRARY KWINEFFECTS_GLUTILS_INCLUDE_DIRS
)
mark_as_advanced (
    KWINEFFECTS_LIBRARY
    KWINEFFECTS_GLUTILS_LIBRARY
)

set (KWINEFFECTS_LIBRARIES ${KWINEFFECTS_LIBRARY})
set (KWINEFFECTS_GLUTILS_LIBRARIES ${KWINEFFECTS_GLUTILS_LIBRARY})

if (KWINEFFECTS_FOUND)
    add_library (KWinEffects::KWinEffects UNKNOWN IMPORTED)
    set_target_properties (KWinEffects::KWinEffects PROPERTIES
        IMPORTED_LOCATION "${KWINEFFECTS_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${KWINEFFECTS_INCLUDE_DIRS}"
    )

    add_library (KWinEffects::GLUtils UNKNOWN IMPORTED)
    set_target_properties (KWinEffects::GLUtils PROPERTIES
        IMPORTED_LOCATION "${KWINEFFECTS_GLUTILS_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${KWINEFFECTS_GLUTILS_INCLUDE_DIRS}"
    )
endif ()
