source ${BUILD_ROOT}/src/linux/build/lib/dep-helper.sh
exit_on_error $?

installer_build_manifest()
{
    local name="${1}"
    local manifest="${2}"
    local native_packages="`product_native_packages ${name}`"
    local native_install_upgrade_package="`product_native_install_upgrade_package ${name}`"
    local native_install_obsolete_packages="`product_native_install_obsolete_packages ${name}`"
    local native_install_base_package="`product_native_install_base_package ${name}`"
    local native_install_gui_package="`product_native_install_gui_package ${name}`"

    mkdir -p "`dirname ${manifest}`"
    
    echo "PREFIX=\"${BUILD_PREFIX}\"" >${manifest}
    echo "PACKAGES=\"${native_packages}\"" >>${manifest}
    echo "INSTALL_UPGRADE_PACKAGE=\"${native_install_upgrade_package}\"" >>${manifest}
    echo "INSTALL_OBSOLETE_PACKAGES=\"${native_install_obsolete_packages}\"" >>${manifest}
    echo "INSTALL_BASE_PACKAGE=\"${native_install_base_package}\"" >>${manifest}
    echo "INSTALL_GUI_PACKAGE=\"${native_install_gui_package}\"" >>${manifest}
}
