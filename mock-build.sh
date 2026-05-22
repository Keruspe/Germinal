#!/usr/bin/env bash

set -euo pipefail

MOCK_CONFIG="${1:-fedora-rawhide-x86_64}"
RESULT_DIR="_build/mock-result"

run_ninja() {
    ninja -C _build "${@}"
}

main() {
    if [[ ! -d _build ]]; then
        meson setup _build
    fi

    run_ninja dist

    mkdir -p ~/rpmbuild/SOURCES
    cp _build/meson-dist/germinal-*.tar.xz ~/rpmbuild/SOURCES/

    rm -rf "${RESULT_DIR}"
    mkdir -p "${RESULT_DIR}"

    mock -r "${MOCK_CONFIG}" \
        --buildsrpm \
        --spec germinal.spec \
        --sources ~/rpmbuild/SOURCES/ \
        --resultdir "${RESULT_DIR}"

    local srpm
    srpm=$(ls "${RESULT_DIR}"/*.src.rpm)

    mock -r "${MOCK_CONFIG}" \
        --rebuild "${srpm}" \
        --resultdir "${RESULT_DIR}"

    echo "RPMs available in ${RESULT_DIR}/"
}

main "${@}"
