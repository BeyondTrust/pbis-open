DEPENDS="core"

# API
# mk_{PACKAGE_TYPE}_do
#   mk_package_files
#   mk_package_dirs
#   mk_subpackage_do
#     mk_package_files
#     mk_package_dirs
#   mk_subpackage_done
# mk_{PACKAGE_TYPE}_done

### section configure

option()
{
    mk_option \
        OPTION="package-dir" \
        VAR="MK_PACKAGE_DIR" \
        PARAM="path" \
        DEFAULT="package" \
        HELP="Subdirectory for built packages"
}

configure()
{
    mk_export MK_PACKAGE_DIR

    mk_add_scrub_target "@package"
}
