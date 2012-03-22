### section configure

lw_unpack()
{
    mk_push_vars ARCHIVE TARGETS DEPS
    mk_parse_params

    mk_quote "@$ARCHIVE"
    DEPS="$DEPS $result"

    mk_target \
        TARGET=".unpack_stamp" \
        DEPS="$DEPS" \
        _lw_unpack '$@' "&.unpack_work" "@$ARCHIVE" "&$TARGETS"

    mk_quote "$result"
    DEPS="$result"

    mk_unquote_list "$TARGETS"
    for TARGET
    do
        mk_target TARGET="$TARGET" DEPS="$DEPS"
    done

    mk_add_clean_target ".unpack_work"

    mk_pop_vars
}

### section build

_lw_unpack()
{
    STAMP="$1"
    WORK="$2"
    ARCHIVE="$3"

    shift 3

    mk_msg_domain "unpack"

    mk_safe_rm "$WORK"
    mk_mkdir "$WORK/image"

    mk_msg "extract $ARCHIVE"

    gunzip < "$ARCHIVE" | { cd "$WORK/image" && tar xf -; } ||
        mk_fail "could not unpack $WORK/stage.tar.gz"

    for TARGET
    do
        mk_msg "${TARGET#$MK_STAGE_DIR}"
        mk_mkdirname "$TARGET"
        mk_run_or_fail mv -f "$WORK/image${TARGET#$MK_STAGE_DIR}" "$TARGET"
    done

    mk_run_or_fail touch "$STAMP"

    mk_safe_rm "$WORK"
}