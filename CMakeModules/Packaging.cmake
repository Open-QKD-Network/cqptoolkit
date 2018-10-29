### @file
### @brief CQP Toolkit
### 
### @copyright Copyright (C) University of Bristol 2016
###    This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. 
###    If a copy of the MPL was not distributed with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
###    See LICENSE file for details.
### @date 04 April 2016
### @author Richard Collins <richard.collins@bristol.ac.uk>
### 
# setup cpack to create installation packages using the generators we support

if(WIN32 AND NOT UNIX)
    LIST(APPEND CPACK_GENERATOR "WIX" "ZIP")
    # Find wix
    SET(WIX_ROOT_DIR "")
    #SET(WIX_FIND_REQUIRED true)
    include(UserFindWix)

    # See https://cmake.org/cmake/help/v3.0/module/CPackWIX.html
    # NOTE for Copy Paste Monkies: Do NOT reuse these GUIDs!
    # ======================================================
    # This number is used by the installer to track different versions of the same product
    # Change this when producing a different product which could be installed along side this.
    SET(CPACK_WIX_UPGRADE_GUID  "2a1312c6-9adf-4998-8df9-f3f552a7af37")
    # Change this when you change the major version number
    SET(CPACK_WIX_PRODUCT_GUID  "53331d41-5ce1-480b-96cf-dfa4a60ff77b")
    if(NOT(${CPACK_WIX_PRODUCT_GUID} STREQUAL "53331d41-5ce1-480b-96cf-dfa4a60ff77b") OR NOT(${BUILD_VERSION_MAJOR} STREQUAL "0"))
        message(FATAL_ERROR "Major version has changed, update product GUID or fix this test.")
    endif()

    SET(CPACK_WIX_LICENSE_RTF   "${CMAKE_SOURCE_DIR}/LICENSE.rtf")
    SET(CPACK_WIX_CMAKE_PACKAGE_REGISTRY "CQPToolkit")
    SET(CPACK_WIX_PRODUCT_ICON  "${CMAKE_SOURCE_DIR}/packaging/QCH_icon.ico")
    SET(CPACK_WIX_UI_BANNER     "${CMAKE_SOURCE_DIR}/packaging/QCH_Banner_White.bmp")
    SET(CPACK_WIX_UI_DIALOG     "${CMAKE_SOURCE_DIR}/packaging/QCH_UI_White.bmp")
    SET(CPACK_WIX_EXTRA_SOURCES "${CMAKE_SOURCE_DIR}/packaging/wix-gui.wxs" 
                                "${CMAKE_SOURCE_DIR}/packaging/CQPSetupTypeDlg.wxs"
                                "${CMAKE_SOURCE_DIR}/packaging/EnvironmentVars.wxs"
                                )
    SET(CPACK_WIX_UI_REF        "CQP_Mondo")

else()
    if(DEB_ENABLED)
        LIST(APPEND CPACK_GENERATOR "DEB")
    endif(DEB_ENABLED)
    # deb settings
    SET(CPACK_DEBIAN_ARCHIVE_TYPE "gnutar")
    SET(CPACK_DEBIAN_PACKAGE_VERSION "${BUILD_VERSION_MAJOR}.${BUILD_VERSION_MINOR}.${BUILD_VERSION_PATCH}")

    # TODO: set exe pcaps with control file
    SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/postinst;${CMAKE_CURRENT_SOURCE_DIR}/triggers")
    # create individual debs for each component
    SET(CPACK_DEB_COMPONENT_INSTALL "ON")
    SET(CPACK_ARCHIVE_COMPONENT_INSTALL "ON")
    SET(CPACK_DEBIAN_ENABLE_COMPONENT_DEPENDS "ON")
    SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS "ON")
    SET(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS "ON")
    SET(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS_POLICY ">=")

    #HACK:
    # again doing this here because at the package level this has no effect
    SET(CPACK_COMPONENT_${PROJECT_NAME}_DESCRIPTION   "CQP Toolkit Tools")
    SET(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://gitlab.com/QComms/cqptoolkit")

    SET(CPACK_DEBIAN_QKDINTERFACES_PACKAGE_DEPENDS "libgrpc++1 (>= 1.10.0)")
    SET(CPACK_DEBIAN_CQPTOOLKIT_PACKAGE_DEPENDS "cqp-qkdinterfaces (>= ${BUILD_VERSION_MAJOR}.${BUILD_VERSION_MINOR})")
    SET(CPACK_DEBIAN_CQPUI_PACKAGE_DEPENDS "cqp-cqptoolkit (>= ${BUILD_VERSION_MAJOR}.${BUILD_VERSION_MINOR})")
    SET(CPACK_DEBIAN_EXAMPLES_PACKAGE_DEPENDS "cqp-cqptoolkit (>= ${BUILD_VERSION_MAJOR}.${BUILD_VERSION_MINOR})")
    SET(CPACK_DEBIAN_TOOLS_PACKAGE_DEPENDS "cqp-cqptoolkit (>= ${BUILD_VERSION_MAJOR}.${BUILD_VERSION_MINOR})")
    SET(CPACK_DEBIAN_TESTS_PACKAGE_DEPENDS "cqp-cqptoolkit (>= ${BUILD_VERSION_MAJOR}.${BUILD_VERSION_MINOR})")
    SET(CPACK_DEBIAN_UI_PACKAGE_DEPENDS "cqp-cqpui (>= ${BUILD_VERSION_MAJOR}.${BUILD_VERSION_MINOR})")

    SET(CPACK_DEBIAN_QKDINTERFACESD_PACKAGE_DEPENDS "libgrpc++1 (>= 1.10.0)")
    SET(CPACK_DEBIAN_CQPTOOLKITD_PACKAGE_DEPENDS "cqp-qkdinterfacesd (>= ${BUILD_VERSION_MAJOR}.${BUILD_VERSION_MINOR})")
    SET(CPACK_DEBIAN_CQPUID_PACKAGE_DEPENDS "cqp-cqptoolkitd (>= ${BUILD_VERSION_MAJOR}.${BUILD_VERSION_MINOR})")
    SET(CPACK_DEBIAN_EXAMPLESD_PACKAGE_DEPENDS "cqp-cqptoolkitd (>= ${BUILD_VERSION_MAJOR}.${BUILD_VERSION_MINOR})")
    SET(CPACK_DEBIAN_TOOLSD_PACKAGE_DEPENDS "cqp-cqptoolkitd (>= ${BUILD_VERSION_MAJOR}.${BUILD_VERSION_MINOR})")
    SET(CPACK_DEBIAN_TESTSD_PACKAGE_DEPENDS "cqp-cqptoolkitd (>= ${BUILD_VERSION_MAJOR}.${BUILD_VERSION_MINOR})")
    SET(CPACK_DEBIAN_UID_PACKAGE_DEPENDS "cqp-cqpuid (>= ${BUILD_VERSION_MAJOR}.${BUILD_VERSION_MINOR})")

    if(RPM_ENABLED)
        LIST(APPEND CPACK_GENERATOR "RPM")
    endif(RPM_ENABLED)
endif()

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY   "CQP Toolkit")
SET(CPACK_PACKAGE_VENDOR                "University of Bristol")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY   "Centre for Quantum Photonics Toolkit")
SET(CPACK_RESOURCE_FILE_LICENSE         "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
SET(CPACK_PACKAGE_VERSION               ${BUILD_VERSION})
SET(CPACK_CYGWIN_PATCH_NUMBER           ${BUILD_VERSION_PATCH})
SET(CPACK_PACKAGE_INSTALL_DIRECTORY     "CQPToolkit")
SET(CPACK_PACKAGE_CONTACT               "Richard.Collins@bristol.ac.uk")

IF(WIN32 AND NOT UNIX)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  SET(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}\\\\packaging/QCH_icon.ico")
ELSE(WIN32 AND NOT UNIX)
  SET(CPACK_STRIP_FILES "bin/MyExecutable")
  SET(CPACK_SOURCE_STRIP_FILES "")
  set(CPACK_RPM_COMPONENT_INSTALL ON)
ENDIF(WIN32 AND NOT UNIX)

# This needs to be included after all variables for it are set
# Changes to variables made after this wont effect CPack
INCLUDE(CPack)

cpack_add_install_type(Runtime DISPLAY_NAME "Minimal Runtime")
cpack_add_install_type(Development DISPLAY_NAME "Development Libraries")

########### Groups ################
cpack_add_component_group(Runtime
    DESCRIPTION     "Prebuild binaries for use in existing programs."
    EXPANDED
    BOLD_TITLE)

cpack_add_component_group(Development
    DESCRIPTION     "Prebuild libraries for use in other programs."
    EXPANDED
    BOLD_TITLE)

cpack_add_component_group(Examples
    DESCRIPTION     "Simple programs demonstrating the use of the libraries"
    )

#cpack_add_component_group(Tests
#    DESCRIPTION     "Programs for performing regression tests on the libraries"
#    )

########## Components ###########
#CPACK_ADD_COMPONENT(Simulate
#    DISPLAY_NAME    "Simulation tools"
#    GROUP           Runtime
#)
#CPACK_ADD_COMPONENT(RunTimeDeps
#    DISPLAY_NAME    "Toolkit Dependencies"
#    GROUP           Runtime
#    DEPENDS         CQPToolkit_Runtime
#    INSTALL_TYPES   Runtime
#)
#
#CPACK_ADD_COMPONENT(CQPToolkit_Runtime
#    DISPLAY_NAME    "CQP Toolkit Runtime"
#    GROUP           Runtime
#    DESCRIPTION     "Install the dynamically loaded library which may be needed by other programs."
#    INSTALL_TYPES   Runtime
#)
#
#CPACK_ADD_COMPONENT(CQPToolkit_Devel
#    DISPLAY_NAME    "CQP Toolkit SDK"
#    GROUP           Development
#    DEPENDS         RunTimeDeps
#    DESCRIPTION     "Install the static library for linking directly to."
#    INSTALL_TYPES   Development
#)

#if(GEN_DOC)
#    CPACK_ADD_COMPONENT(CQP_Docs
#        DISPLAY_NAME    "Documentation"
#        DESCRIPTION     "Toolkit library documentation in html."
#        GROUP           Development
#        INSTALL_TYPES   Development
#        DEPENDS         doc
#    )
#endif(GEN_DOC)
#
#if(BUILD_CQP_EXAMPLES)
#    CPACK_ADD_COMPONENT(NewDriver_Devel
#        DISPLAY_NAME    "Driver example prgram"
#        GROUP           Examples
#        DEPENDS         CQPToolkit_Devel
#        INSTALL_TYPES   Development
#    )
#
#    CPACK_ADD_COMPONENT(NewDriver_Runtime
#        DISPLAY_NAME    "Driver example prgram"
#        GROUP           Examples
#        DEPENDS         CQPToolkit_Runtime
#        INSTALL_TYPES   Development
#    )
#
#endif(BUILD_CQP_EXAMPLES)
#
#if(BUILD_TESTING)
#    CPACK_ADD_COMPONENT(DllArchBridgeTests
#        DISPLAY_NAME    "Dll ArchBridge Tests"
#        DESCRIPTION     "Run specific tests on the toolkit."
#        GROUP           Tests
#        DEPENDS         CQPToolkit_Runtime
#        INSTALL_TYPES   Runtime Development
#    )
#
#    CPACK_ADD_COMPONENT(CQPDriverTests
#        DISPLAY_NAME    "CQP Driver Tests"
#        DESCRIPTION     "Run specific tests on the toolkit."
#        GROUP           Tests
#        DEPENDS         CQPToolkit_Runtime
#        INSTALL_TYPES   Runtime Development
#    )
#
#    CPACK_ADD_COMPONENT(CQPInterfaceTests
#        DISPLAY_NAME    "CQP Interface Tests"
#        DESCRIPTION     "Run specific tests on the toolkit."
#        GROUP           Tests
#        DEPENDS         CQPToolkit_Runtime
#        INSTALL_TYPES   Runtime Development
#    )
#
#    CPACK_ADD_COMPONENT(CQPRemoteObjectTests
#        DISPLAY_NAME    "CQP RemoteObject Tests"
#        DESCRIPTION     "Run specific tests on the toolkit."
#        GROUP           Tests
#        DEPENDS         CQPToolkit_Runtime
#        INSTALL_TYPES   Runtime Development
#    )
#
#    CPACK_ADD_COMPONENT(CQPUtilTests
#        DISPLAY_NAME    "CQP Utility Tests"
#        DESCRIPTION     "Run specific tests on the toolkit."
#        GROUP           Tests
#        DEPENDS         CQPToolkit_Runtime
#        INSTALL_TYPES   Runtime Development
#    )
#endif(BUILD_TESTING)

########### Special build steps for making the bundle (not supported by CPack)

if(WIX_FOUND)
    MACRO(DOWNLOAD_FILE _prefix _url _filename _md5)

        SET(${_prefix}_DL_FILE "${_filename}")
        SET(${_prefix}_URL "${_url}")

        add_custom_target(Get${_prefix}
            # Options must come before the path!
            COMMAND ${CMAKE_COMMAND} -DDOWNLOAD_URL=${${_prefix}_URL} -DDOWNLOAD_FILE=SourceDir/${${_prefix}_DL_FILE} -DDOWNLOAD_MD5=${_md5} -P "${CMAKE_SOURCE_DIR}/CMakeModules/Download.cmake" 
        )

        if(MSVC)
            set_property(TARGET Get${_prefix} PROPERTY FOLDER "Packaging")
        endif(MSVC)
        LIST(APPEND BUNDLE_DEPS Get${_prefix})
    ENDMACRO(DOWNLOAD_FILE _prefix _url _md5)

    SET(BUNDLE_DEPS "")
    SET(WIX_CANDLE_FLAGS "-ext" "WixBalExtension" "-ext" "WixUtilExtension")
    SET(WIX_LINK_FLAGS ${WIX_CANDLE_FLAGS})

    # Get the msi - they need to exist so that wix can scan them
    DOWNLOAD_FILE(CMake "https://cmake.org/files/v3.9/cmake-3.9.0-win64-x64.msi" cmake.msi 8c27719b88e82ca876f40d2931ebbc95)
    DOWNLOAD_FILE(TortoiseSVN "https://vorboss.dl.sourceforge.net/project/tortoisesvn/1.9.6/Application/TortoiseSVN-1.9.6.27867-x64-svn-1.9.6.msi" TortoiseSVN.msi accb7458887046b193b422e5344b482b)

    DOWNLOAD_FILE(CodeBlocks "http://sourceforge.net/projects/codeblocks/files/Binaries/16.01/Windows/codeblocks-16.01-setup.exe" codeblocks.exe 68c0930c16c1c4418bcc82daf9f01d47)
    DOWNLOAD_FILE(MSYS2 "http://repo.msys2.org/distrib/x86_64/msys2-x86_64-20161025.exe" msys2.exe 0be5539d68dc5533ad387e06dfa08abf)
    
    DOWNLOAD_FILE(CppCheck "http://github.com/danmar/cppcheck/releases/download/1.80/cppcheck-1.80-x64-Setup.msi" cppcheck.msi 2255be9ea0964154ff831fa21e80e8ba)
    DOWNLOAD_FILE(Doxygen "http://ftp.stack.nl/pub/users/dimitri/doxygen-1.8.13-setup.exe" doxygen.exe 91e3afa8d76f15b0cfab43f2615b1c88)
    DOWNLOAD_FILE(Graphviz "http://www.graphviz.org/pub/graphviz/stable/windows/graphviz-2.38.msi" graphviz.msi 29b363f9d6c9cbaaac667deab8edb373)
    #Intel SDK for openCL
    
    SET(BUNDLE_FILES "${CMAKE_SOURCE_DIR}/packaging/Bundle.wxs") 
    SET(BUNDLE_MSI "${PROJECT_NAME}_Build_Env-${BUILD_VERSION}.exe")

    WIX_COMPILE(BUNDLE_FILES BUNDLE_OBJ BUNDLE_DEPS)
    WIX_LINK("${BUNDLE_MSI}" BUNDLE_OBJ "")
    
    add_custom_target(bundle
        DEPENDS doc ${BUNDLE_MSI})

    if(MSVC)
        set_property(TARGET bundle PROPERTY FOLDER "Packaging")
        set_property(TARGET bundle PROPERTY EXCLUDE_FROM_DEFAULT_BUILD 1)
    endif(MSVC)

endif(WIX_FOUND)
########### Pull it all together in a build step
