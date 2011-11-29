_mk_build_lpp()
{
    # $1 = output directory
    # $2 = package version
    # $3 = lpp directory
    # ... = subpackages

    LPP_OUTPUT="$1"
    LPP_VER="$2"
    LPP_DIR="$3"
    LPP_NAME="${LPP_OUTPUT#$MK_PACKAGE_LPP_DIR/}"
    LPP_NAMEDOT=`echo "$LPP_NAME" | tr '-' '.'`

    shift 3

    mk_msg_domain "lpp"

    mk_msg "begin $LPP_NAME"

    mk_msg "$LPP_NAMEDOT filesystem"

    # Build filesystem image
    _mk_lpp_filesystem "$@"

    mk_msg "$LPP_NAMEDOT metadata"

    # Build .size files
    _mk_lpp_size "$@"

    # Build .inventory files
    _mk_lpp_inventory "$@"

    # Build .al files
    _mk_lpp_al "$@"

    # Build liblpp.a files
    _mk_lpp_libs

    # Build lpp_name file
    _mk_lpp_name "$@"

    mk_msg "$LPP_NAMEDOT bff"

    # Build the bff
    _mk_lpp_bff "$@"

    mk_msg "end $LPP_NAME"
}

_mk_lpp_filesystem()
{
    mk_safe_rm "$LPP_DIR/image"

    mk_mkdir "${LPP_DIR}/image/usr/lpp/${LPP_NAMEDOT}"
    mk_mkdir "${LPP_DIR}/image/usr/lpp/${LPP_NAMEDOT}/inst_root"

    for LPP_SUB
    do
        LPP_MANIFEST="${LPP_DIR}/${LPP_SUB}"

        {
            while mk_read_line
            do
                mk_mkdir "${LPP_DIR}/image/usr/lpp/${LPP_NAMEDOT}/inst_root${result}"
            done
        } < "$LPP_MANIFEST.root.dirs"

        {
            while mk_read_line
            do
                mk_mkdir "${LPP_DIR}/image${result}"
            done
        } < "$LPP_MANIFEST.usr.dirs"

        {
            while mk_read_line
            do
                DEST="${LPP_DIR}/image/usr/lpp/${LPP_NAMEDOT}/inst_root${result}"
                SRC="${MK_STAGE_DIR}${result}"
                mk_mkdirname "${DEST}"
                _mk_lpp_transfer "$SRC" "$DEST"
            done
        } < "$LPP_MANIFEST.root.files"

        {
            while mk_read_line
            do
                DEST="${LPP_DIR}/image${result}"
                SRC="${MK_STAGE_DIR}${result}"
                mk_mkdirname "${DEST}"
                _mk_lpp_transfer "$SRC" "$DEST"
            done
        } < "$LPP_MANIFEST.usr.files"
    done
}

_mk_lpp_transfer()
{
    mk_run_or_fail cp -RP "$1" "$2"

    if [ "$MK_DEBUG" = "no" ]
    then
        case "`file -h "$2"`" in
            *"64-bit"*"not stripped")
                mk_get_file_mode "$2"
                _old_mode="$result"
                mk_run_or_fail chmod u+w "$2"
                mk_run_or_fail strip -X64 "$2"
                mk_run_or_fail chmod "$_old_mode" "$2"
                ;;
            *"not stripped")
                mk_get_file_mode "$2"
                _old_mode="$result"
                mk_run_or_fail chmod u+w "$2"
                mk_run_or_fail strip "$2"
                mk_run_or_fail chmod "$_old_mode" "$2"
                ;;
        esac
    fi
}

_mk_lpp_size()
{
    for LPP_SUB
    do
        LPP_MANIFEST="${LPP_DIR}/${LPP_SUB}"

        {
            sed 's:/[^/]*$::g' < "$LPP_MANIFEST.usr.files"
            sed -e "s:^:/usr/lpp/${LPP_NAMEDOT}/inst_root:g" \
                -e 's:/[^/]*$::g' < "$LPP_MANIFEST.root.files"
        } | sort | uniq | tee "$LPP_MANIFEST.usr.dirlist" |
        {
            while mk_read_line
            do
                DIR="$result"
                REALDIR="$result"
                case "$DIR" in
                    "/usr/lpp/${LPP_NAMEDOT}/inst_root/"*)
                        REALDIR="${DIR#/usr/lpp/${LPP_NAMEDOT}/inst_root}"
                        TOTAL=$(grep "^${REALDIR}/[^/][^/]*\$" "$LPP_MANIFEST.root.files" |
                            sed "s:^:${LPP_DIR}/image/usr/lpp/${LPP_NAMEDOT}/inst_root:g" |
                            xargs du | awk '{total += $1; next} END { print total; }')
                        ;;
                    *)
                        TOTAL=$(grep "^${DIR}/[^/][^/]*\$" "$LPP_MANIFEST.usr.files" |
                            sed "s:^:${LPP_DIR}/image:g" |
                            xargs du | awk '{total += $1; next} END { print total; }')
                        ;;
                esac
                echo "$DIR $TOTAL"
            done
        } > "$LPP_MANIFEST.usr.size"

        # Generate the root size file by grepping for inst_root stuff
        grep "^/usr/lpp/${LPP_NAMEDOT}/inst_root/" "$LPP_MANIFEST.usr.size" |
        sed "s:^/usr/lpp/${LPP_NAMEDOT}/inst_root::g" > "$LPP_MANIFEST.root.size"
        
        # Now add any upsizes to the usr portion
        [ -f "$LPP_MANIFEST.upsize" ] && cat "$LPP_MANIFEST.upsize" >> "$LPP_MANIFEST.usr.size"
        
        # Count total files
        FILE_TOTAL=$(wc -l "$LPP_MANIFEST.usr.files" "$LPP_MANIFEST.root.files" | tail -1 | awk '{print $1;}')
        # Total config file size
        CONFIG_TOTAL=$(du -a "$LPP_MANIFEST".* | awk '{total += $1; next} END { print total; }')
        
        echo "/usr/lib/objrepos $FILE_TOTAL" >> "$LPP_MANIFEST.usr.size"
        echo "/etc/objrepos $FILE_TOTAL" >> "$LPP_MANIFEST.root.size"
        echo "INSTWORK $CONFIG_TOTAL $CONFIG_TOTAL" >> "$LPP_MANIFEST.usr.size"
        echo "INSTWORK $CONFIG_TOTAL $CONFIG_TOTAL" >> "$LPP_MANIFEST.root.size"
    done
}

_mk_lpp_inventory()
{
    for LPP_SUB
    do
        LPP_MANIFEST="${LPP_DIR}/${LPP_SUB}"

        {
            cat "$LPP_MANIFEST.usr.dirs"
            sed "s:^:/usr/lpp/${LPP_NAMEDOT}/inst_root:g" < "$LPP_MANIFEST.root.dirs"
            cat "$LPP_MANIFEST.usr.files"
            sed "s:^:/usr/lpp/${LPP_NAMEDOT}/inst_root:g" < "$LPP_MANIFEST.root.files"
        } | _mk_lpp_emit_inventory > "${LPP_MANIFEST}.usr.inventory"

        {
            sed "s:^:/usr/lpp/${LPP_NAMEDOT}/inst_root:g" < "$LPP_MANIFEST.root.dirs"
            sed "s:^:/usr/lpp/${LPP_NAMEDOT}/inst_root:g" < "$LPP_MANIFEST.root.files"
        } | _mk_lpp_emit_inventory | sed "s:^/usr/lpp/${LPP_NAMEDOT}/inst_root::g" > "${LPP_MANIFEST}.root.inventory"
    done
}

_mk_lpp_emit_inventory()
{
    while mk_read_line
    do
        FILE="$result"
        mk_get_file_mode "$LPP_DIR/image${FILE}"
        MODE="$result"
        
        echo "$FILE:"
        echo "          owner = root"
        echo "          group = system"
        echo "          mode = $MODE"
        
        if [ -d "$LPP_DIR/image${FILE}" ]
        then
            echo "          type = DIRECTORY"
            echo "          class = apply,inventory,${LPP_NAMEDOT}.${LPP_SUB}"
        elif [ -h "$LPP_DIR/image${FILE}" ]
        then
            TARGET=$(file -h "$LPP_DIR/image${FILE}")
            TARGET="${TARGET#* symbolic link to }"
            TARGET="${TARGET%.}"
            
            echo "          type = SYMLINK"
            echo "          class = apply,inventory,${LPP_NAMEDOT}.${LPP_SUB}"
            echo "          target = ${TARGET}"
        else
            SIZE=$(ls -l "$LPP_DIR/image${FILE}" | awk '{print $5;}')
            CKSUM=$(sum -r "$LPP_DIR/image${FILE}" | awk '{print $1 " " $2;}')
            
            echo "          type = FILE"
            echo "          class = apply,inventory,${LPP_NAMEDOT}.${LPP_SUB}"
            echo "          size = $SIZE"
            echo "          checksum = \"$CKSUM\""
        fi
        
        echo ""
    done
}

_mk_lpp_al()
{
    for LPP_SUB
    do
        LPP_MANIFEST="${LPP_DIR}/${LPP_SUB}"

        {
            sed "s:^:.:g" < "$LPP_MANIFEST.usr.dirs"
            sed "s:^:./usr/lpp/${LPP_NAMEDOT}/inst_root:g" < "$LPP_MANIFEST.root.dirs"
            sed "s:^:.:g" < "$LPP_MANIFEST.usr.files"
            sed "s:^:./usr/lpp/${LPP_NAMEDOT}/inst_root:g" < "$LPP_MANIFEST.root.files"
        } > "$LPP_MANIFEST.usr.al"

        {
            sed "s:^:.:g" < "$LPP_MANIFEST.root.dirs"
            sed "s:^:.:g" < "$LPP_MANIFEST.root.files"
        } > "$LPP_MANIFEST.root.al"
    done
}

_mk_lpp_lib()
{
    mk_safe_rm "$LPP_DIR/liblpp.$2"
    mk_mkdir "$LPP_DIR/liblpp.$2"
    
    if [ "$2" = "root" ]
    then
        CONFIG_LIST="config unconfig pre_u post_i"
    else
        CONFIG_LIST="pre_i post_u pre_rm"
    fi

    for CONFIG in ${CONFIG_LIST}
    do
        for FILE in "$LPP_DIR/"*".$CONFIG"
        do
            [ -f "$FILE" ] &&
            mk_run_or_fail cp "$FILE" "$LPP_DIR/liblpp.$2/$LPP_NAMEDOT.${FILE##*/}"
        done
    done
    
    for CONFIG in size inventory al
    do
        for FILE in "$LPP_DIR/"*".$2.$CONFIG"
        do
            if [ -f "$FILE" ]
            then
                DEST="${FILE##*/}"
                DEST="${DEST%.$2.$CONFIG}.$CONFIG"
                mk_run_or_fail cp "$FILE" "$LPP_DIR/liblpp.$2/$LPP_NAMEDOT.$DEST"
            fi
        done
    done

    mk_cd_or_fail "$LPP_DIR/liblpp.$2"
    mk_run_or_fail ar -clgr "liblpp.a" *
    mk_cd_or_fail "$MK_ROOT_DIR"
    mk_run_or_fail mv "$LPP_DIR/liblpp.$2/liblpp.a" "$1"
}

_mk_lpp_libs()
{
    _mk_lpp_lib "${LPP_DIR}/image/usr/lpp/${LPP_NAMEDOT}/" "usr"
    _mk_lpp_lib "${LPP_DIR}/image/usr/lpp/${LPP_NAMEDOT}/inst_root/" "root"
}

_mk_lpp_name()
{
    {
        echo "4 R I ${LPP_NAMEDOT} {"
        for LPP_SUB
        do
            LPP_MANIFEST="${LPP_DIR}/${LPP_SUB}"
            cat "${LPP_MANIFEST}.name"
            echo "["
            [ -f "${LPP_MANIFEST}.requisites" ] && cat "${LPP_MANIFEST}.requisites"
            echo "%"
            cat "$LPP_MANIFEST.usr.size" "$LPP_MANIFEST.root.size" | sort | uniq
            echo "%"
            echo "%"
            echo "%"
            echo "]"
        done
        echo "}"
    } > "${LPP_DIR}/image/lpp_name"
}

_mk_lpp_bff()
{
    mk_mkdir "$LPP_OUTPUT"

    {
        echo "./"
        echo "./lpp_name"
        echo "./usr"
        echo "./usr/lpp"
        echo "./usr/lpp/$LPP_NAMEDOT"
        echo "./usr/lpp/$LPP_NAMEDOT/liblpp.a"
        echo "./usr/lpp/$LPP_NAMEDOT/inst_root"
        echo "./usr/lpp/$LPP_NAMEDOT/inst_root/liblpp.a"
        for LPP_SUB
        do
            LPP_MANIFEST="${LPP_DIR}/${LPP_SUB}"
            cat "${LPP_MANIFEST}.usr.al"
        done
    } | tee "$LPP_DIR/image-list" | 
    {
        mk_cd_or_fail "$LPP_DIR/image"
        mk_run_or_fail backup -iqprf "$MK_ROOT_DIR/$LPP_OUTPUT/${LPP_NAMEDOT}-${LPP_VER}.bff" -b1
    }

    mk_msg "built ${LPP_NAMEDOT}-${LPP_VER}.bff"
}

_mk_build_lpp "$@"
