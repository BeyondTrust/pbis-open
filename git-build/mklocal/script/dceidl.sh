#!/bin/sh

MK_MSG_DOMAIN="dceidl"

if [ -z "$DCEIDL" ]
then
    DCEIDL="${MK_RUN_BINDIR}/dceidl"
fi

if [ -n "$HEADER" ]
then
    OPT_HEADER="-header ${MK_OBJECT_DIR}${MK_SUBDIR}/$HEADER"
fi

if [ -n "$CSTUB" ]
then
    OPT_CSTUB="-client all -cstub ${MK_OBJECT_DIR}${MK_SUBDIR}/$CSTUB"
else
    OPT_CSTUB="-client none"
fi

if [ -n "$SSTUB" ]
then
    OPT_SSTUB="-server all -sstub ${MK_OBJECT_DIR}${MK_SUBDIR}/$SSTUB"
else
    OPT_SSTUB="-server none"
fi

OPT_INCLUDES="-I${MK_STAGE_DIR}${MK_INCLUDEDIR}"

for dir in ${INCLUDEDIRS}
do
    OPT_INCLUDES="$OPT_INCLUDES -I${MK_SOURCE_DIR}${MK_SUBDIR}/$dir -I${MK_OBJECT_DIR}${MK_SUBDIR}/$dir"
done

mk_msg "${1#${MK_SOURCE_DIR}/}"
mk_run_or_fail \
    ${DCEIDL} \
    -keep c_source \
    ${OPT_HEADER} \
    ${OPT_CSTUB} \
    ${OPT_SSTUB} \
    ${OPT_INCLUDES} \
    ${IDLFLAGS} \
    "$@" || mk_fail "dceidl failed"
