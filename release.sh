#!/usr/bin/env bash

set -euo pipefail

run_ninja() {
    ninja -C _build "${@}"
}

main() {
    local version="${1}"
    appstreamcli validate data/metainfo/*.xml.in || exit 1
    run_ninja
    ls po/*.po | sed 's|po/||; s|\.po$||' | sort > po/LINGUAS
    run_ninja Germinal-pot
    run_ninja Germinal-update-po
    git commit -asm "Release Germinal ${version}"
    run_ninja dist
    git tag -sm "Release Germinal ${version}" v${version}
}

main "${@}"
