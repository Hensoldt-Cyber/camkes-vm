#
# Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

cmake_minimum_required(VERSION 3.8.2)

set(VM_PROJECT_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "")
if(NOT TARGET vm_fserver_config)
    add_custom_target(vm_fserver_config)
endif()

# Function for declaring a CAmkESVM. This is called for each Init component in the applications
# camkes config.
# init_component: is the name of CamkESVM Init component described in the .camkes config
# In addition the user can pass in extra compilation sources, includes, libs and flags through
# the SOURCES, INCLUDES, LIBS, LD_FLAGS and C_FLAGS arguments.
function(DeclareCAmkESVM init_component)
    cmake_parse_arguments(
        PARSE_ARGV
        1
        VM_COMP
        ""
        ""
        "EXTRA_SOURCES;EXTRA_INCLUDES;EXTRA_LIBS;EXTRA_C_FLAGS;EXTRA_LD_FLAGS"
    )
    # Retrieve sources for Init component
    file(GLOB base_sources ${VM_PROJECT_DIR}/components/Init/src/*.c)
    # Declare an Init component for CAmkESVM
    DeclareCAmkESComponent(
        ${init_component}
        SOURCES
        ${base_sources}
        ${VM_COMP_EXTRA_SOURCES}
        INCLUDES
        ${VM_PROJECT_DIR}/components/Init/src
        ${VM_PROJECT_DIR}/components/Init/include
        ${VM_PROJECT_DIR}/components/VM/configurations
        ${VM_COMP_EXTRA_INCLUDES}
        LIBS
        sel4allocman
        sel4vm
        sel4vmmplatsupport
        sel4_autoconf
        camkes_vmm_Config
        virtqueue
        vswitch
        FileServer-client
        ${VM_COMP_EXTRA_LIBS}
        LD_FLAGS
        ${VM_COMP_EXTRA_LD_FLAGS}
        C_FLAGS
        ${VM_COMP_EXTRA_C_FLAGS}
        TEMPLATE_SOURCES
        seL4ExtraRAM.template.c
        seL4ExcludeGuestPAddr.template.c
        seL4InitConnection.template.c
        seL4VMIOPorts.template.c
        seL4GuestMaps.template.c
        seL4VMIRQs.template.c
        seL4VMPCIDevices.template.c
    )
endfunction(DeclareCAmkESVM)

#
# Function defines a CAmkESVMFileServer using the declared fileserver images
# and fileserver dependencies. These images are placed into a CPIO archive.
#
# Parameters:
#
# TYPE <type>
#   Type of the file server CAmkES component.
#   Optional, defaults to "FileServer"
#
# INSTANCE <name>
#   Instance name of the file server CAmkES component.
#   Optional, defaults to "fserv"
#
# FILES [<CPIO_NAME>:]<FILE_NAME>[ <CPIO_NAME>:]<FILE_NAME>[...]]
#  Files to be added to the file server.
#
# DEPENDS <dep>[ <dep>[...]]
#   Any additional dependencies for the file/image the caller is adding to the
#   file server
#
#
function(DefineCAmkESVMFileServer)

    cmake_parse_arguments(
        PARSE_ARGV
        0
        PARAM # variable prefix
        "" # option arguments
        "TYPE;INSTANCE" # optional single value arguments
        "FILES;DEPENDS" # optional multi value arguments
    )

    if(PARAM_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: ${PARAM_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT PARAM_TYPE)
        set(PARAM_TYPE "FileServer")
    endif()

    if(NOT PARAM_INSTANCE)
        set(PARAM_INSTANCE "fserv")
    endif()

    if(PARAM_DEPENDS)
        set_property(
            TARGET vm_fserver_config
            APPEND
            PROPERTY DEPS_${PARAM_INSTANCE} ${PARAM_DEPENDS}
        )
    endif()

    foreach(item IN LISTS PARAM_FILES) # [<CPIO_NAME>:]<FILE_NAME>
        string(
            REPLACE
                ":"
                ";"
                item_as_list
                "${item}"
        )
        list(LENGTH item_as_list len)
        if(len EQUAL 2)
            list(POP_FRONT item_as_list CPIO_NAME)
            list(POP_FRONT item_as_list FILE_NAME)
        elseif(len EQUAL 1)
            list(POP_FRONT item_as_list FILE_NAME)
            get_filename_component(CPIO_NAME "${FILE_NAME}" NAME)
        else()
            message(FATAL_ERROR "invalid format: ${item}")
        endif()
        # No need to pass dependencies here, we have already added them above.
        AddToFileServer("${CPIO_NAME}" "${FILE_NAME}" INSTANCE "${PARAM_INSTANCE}")
    endforeach()

    # now process the file/deps list
    get_target_property(files vm_fserver_config FILES_${PARAM_INSTANCE})
    if(NOT files) # this also catches "files-NOTFOUND" if property is not set
        set(files "")
    endif()
    get_target_property(deps vm_fserver_config DEPS_${PARAM_INSTANCE})
    if(NOT deps) # this also catches "deps-NOTFOUND" if property is not set
        set(deps "")
    endif()

    set(INST_BIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/${PARAM_INSTANCE}")

    set(CPIO_FILES "")
    foreach(item IN LISTS files) # [<CPIO_NAME>:]<FILENAME>
        string(
            REPLACE
                ":"
                ";"
                item_as_list
                "${item}"
        )
        list(POP_FRONT item_as_list CPIO_NAME)
        list(POP_FRONT item_as_list FILE_NAME)
        set(cp_target "copy_${PARAM_INSTANCE}_${CPIO_NAME}")
        set(CPIO_FILE "${INST_BIN_DIR}/files/${CPIO_NAME}")
        add_custom_command(
            OUTPUT "${CPIO_FILE}"
            COMMENT "copy: ${FILE_NAME} -> ${CPIO_FILE}"
            COMMAND
                ${CMAKE_COMMAND} -E copy "${FILE_NAME}" "${CPIO_FILE}"
            VERBATIM
            DEPENDS ${FILE_NAME} ${deps}
        )
        add_custom_target(${cp_target} DEPENDS "${CPIO_FILE}")
        list(APPEND CPIO_FILES "${CPIO_FILE}")
    endforeach()

    # Build CPIO archive. Unfortunately MakeCPIO() support plain file names only
    # and does not support paths. Archive will be created in the built output
    # root folder, where using BIN_DIR would be a bit cleaner actually.
    set(CPIO_ARCHIVE "${PARAM_INSTANCE}_cpio_archive.o")
    include(cpio)
    MakeCPIO(${CPIO_ARCHIVE} "${CPIO_FILES}" DEPENDS "${deps}")

    # Build a library from the CPIO archive
    set(FILESERVER_LIB "${PARAM_INSTANCE}_cpio")
    add_library(${FILESERVER_LIB} STATIC EXCLUDE_FROM_ALL ${CPIO_ARCHIVE})
    set_target_properties(
        ${FILESERVER_LIB}
        PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY
            "${INST_BIN_DIR}"
            LINKER_LANGUAGE
            "C"
    )

    # Add the CPIO-library to the component
    ExtendCAmkESComponentInstance("${PARAM_TYPE}" "${PARAM_INSTANCE}" LIBS "${FILESERVER_LIB}")

endfunction(DefineCAmkESVMFileServer)

# Function for declaring the CAmkESVM root server. Taking the camkes application
# config file we declare a CAmkES Root server and the VM File Server. It is
# expected the caller has declared the file server images before using this
# function.
# camkes_config: The applications .camkes file
# In addition the user can pass in extra CPP compilation includes and flags through
# the CPP_INCLUDES and CPP_FLAGS arguments.
function(DeclareCAmkESVMRootServer camkes_config)
    cmake_parse_arguments(PARSE_ARGV 1 CAMKES_ROOT_VM "" "" "CPP_INCLUDES;CPP_FLAGS")
    # Initialise the CAmKES VM fileserver
    DefineCAmkESVMFileServer()
    get_absolute_source_or_binary(config_file "${camkes_config}")
    # Declare CAmkES root server
    DeclareCAmkESRootserver(
        ${config_file}
        CPP_FLAGS
        ${CAMKES_ROOT_VM_CPP_FLAGS}
        CPP_INCLUDES
        "${VM_PROJECT_DIR}/components/VM"
        ${CAMKES_ROOT_VM_CPP_INCLUDES}
    )
endfunction(DeclareCAmkESVMRootServer)

#
# Function for adding a file/image to the vm file server.
#
# Parameters:
#
# <filename_pref>
#   The name the caller wishes to use to reference the file in the CPIO archive.
#   This corresponds with the name set in the 'kernel_image' camkes variable for
#   a given instance vm.
#
# <file_dest>
#   The location of the file/image the caller is adding to the file server
#
# INSTANCE <name>
#   Instance name of the file server CAmkES component.
#   Optional, defaults to "fserv"
#
# DEPENDS <dep>[ <dep>[...]]
#   Any additional dependencies for the file/image the caller is adding to the
#   file server
#
function(AddToFileServer filename_pref file_dest)

    cmake_parse_arguments(
        PARSE_ARGV
        2
        PARAM # variable prefix
        "" # option arguments
        "INSTANCE" # optional single value arguments
        "DEPENDS" # optional multi value arguments
    )

    if(PARAM_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: ${PARAM_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT PARAM_INSTANCE)
        set(PARAM_INSTANCE "fserv")
    endif()

    set_property(
        TARGET vm_fserver_config
        APPEND
        PROPERTY FILES_${PARAM_INSTANCE} "${filename_pref}:${file_dest}"
    )

    if(PARAM_DEPENDS)
        set_property(
            TARGET vm_fserver_config
            APPEND
            PROPERTY DEPS_${PARAM_INSTANCE} ${PARAM_DEPENDS}
        )
    endif()

endfunction(AddToFileServer)

# Function for decompressing/extracting a vmlinux file from a given kernel image
# decompress_target: The target name the caller wishes to use to generate the decompressed kernel
# image
# decompressed_kernel_image: caller variable which is set with the decompressed kernel image location
# compressed_kernel_image: The location of the compressed kernel image
# DEPENDS: Any additional dependencies for the compressed kernel image
function(DecompressLinuxKernel decompress_target decompressed_kernel_image compressed_kernel_image)
    # Get any existing dependencies for decompressing linux kernel
    cmake_parse_arguments(PARSE_ARGV 3 DECOMPRESS_KERNEL "" "" "DEPENDS")
    if(NOT "${DECOMPRESS_KERNEL_UNPARSED_ARGUMENTS}" STREQUAL "")
        message(FATAL_ERROR "Unknown arguments to DecompressLinuxKernel")
    endif()
    # Retrieve filename from kernel path
    get_filename_component(kernel_basename ${compressed_kernel_image} NAME)
    # Extract vmlinux from bzimage
    add_custom_command(
        OUTPUT decomp/${kernel_basename}
        COMMAND
            bash -c
            "${VM_PROJECT_DIR}/tools/elf/extract-vmlinux ${compressed_kernel_image} > decomp/${kernel_basename}"
        VERBATIM
        DEPENDS ${compressed_kernel_image} ${DECOMPRESS_KERNEL_DEPENDS}
    )
    # Create custom target for extraction
    add_custom_target(
        ${decompress_target}
        DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/decomp/${kernel_basename}"
    )
    # Set parameter to tell the caller location of decompressed kernel image
    set(
        ${decompressed_kernel_image} ${CMAKE_CURRENT_BINARY_DIR}/decomp/${kernel_basename}
        PARENT_SCOPE
    )
endfunction(DecompressLinuxKernel)
