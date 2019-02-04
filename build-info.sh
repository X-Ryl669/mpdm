#!/bin/sh

if [ -d .git ] ; then
    GIT_REV=$(git rev-parse HEAD)
else
    GIT_REV="unknown"
fi

TIMESTAMP=$(date +%Y%m%d%H%M%S)

echo "char *${1}_build_info_git_rev = \"${GIT_REV}\";" > .build-info.c
echo "char *${1}_build_info_time = \"${TIMESTAMP}\";" >> .build-info.c
