#
# Bareflank Hypervisor
# Copyright (C) 2015 Assured Information Security, Inc.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

if(ENABLE_BUILD_EFI AND NOT WIN32)
    message(STATUS "Including dependency: acpica")

    download_dependency(
        acpica
        URL         ${ACPICA_URL}
        URL_MD5     ${ACPICA_URL_MD5}
    )

    add_dependency(
        acpica userspace
        PATCH_COMMAND sed -i "s/efi_main/ac_efi_main/g" ${CACHE_DIR}/acpica/source/os_specific/efi/oseficlib.c
        CONFIGURE_COMMAND   ${CMAKE_COMMAND} -E copy_directory ${CACHE_DIR}/acpica/source ${DEPENDS_DIR}/acpica
        BUILD_COMMAND ls
        INSTALL_COMMAND ls
    )

endif()
