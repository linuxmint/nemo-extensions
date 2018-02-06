#!/bin/bash

SRC_ROOT=$1
OUTPUTDIR=$2
PACKAGE_VERSION=$3
DATADIR=$4

pushd "${SRC_ROOT}"

python2 serializeimages.py      \
    "${PACKAGE_VERSION}"        \
    "${datadir}/applications"   \
    < dropbox.in \
    > "dropbox"

python2 docgen.py               \
    "${PACKAGE_VERSION}"        \
    < dropbox.txt.in \
    > "${OUTPUTDIR}/dropbox.txt"

python2 rst2man.py \
    "${OUTPUTDIR}/dropbox.txt" \
    > "${OUTPUTDIR}/dropbox.1"

chmod +x "dropbox"
mv       "dropbox" "${OUTPUTDIR}"

popd
