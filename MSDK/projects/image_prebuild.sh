#!/bin/bash
MSDK_DIR=$2
echo "/* Do not change the content here, it's auto generated */" > ${MSDK_DIR}/app/_build_date.h

DATE=$(date +"%Y/%m/%d-%T")
echo \#define SDK_BUILD_DATE  \"$DATE\" >> ${MSDK_DIR}/app/_build_date.h