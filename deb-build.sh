#!/usr/bin/env bash

set -euo pipefail

require_command() {
    local command="${1}"

    if ! command -v "${command}" >/dev/null 2>&1; then
        echo "Missing required command: ${command}" >&2
        exit 1
    fi
}

meson_version() {
    sed -n "s/^[[:space:]]*version:[[:space:]]*'\\([^']*\\)'.*/\\1/p" meson.build | head -n1
}

merge_depends() {
    local shlibs_depends="${1}"
    local package_depends="gsettings-desktop-schemas (>= 3.28.1), dconf-gsettings-backend | gsettings-backend"

    if [[ -n "${shlibs_depends}" ]]; then
        printf '%s, %s\n' "${shlibs_depends}" "${package_depends}"
    else
        printf '%s\n' "${package_depends}"
    fi
}

main() {
    local source_dir
    source_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    cd "${source_dir}"

    require_command meson
    require_command ninja
    require_command dpkg
    require_command dpkg-deb
    require_command dpkg-shlibdeps

    local package="germinal"
    local version
    version="$(meson_version)"
    if [[ -z "${version}" ]]; then
        echo "Unable to determine project version from meson.build" >&2
        exit 1
    fi

    local deb_revision="${DEB_REVISION:-1local$(date +%Y%m%d%H%M%S)}"
    local deb_version="${DEB_VERSION:-${version}-${deb_revision}}"
    local arch="${DEB_ARCH:-$(dpkg --print-architecture)}"

    local build_dir="${BUILD_DIR:-_build/deb-build}"
    local work_dir="${WORK_DIR:-_build/deb-work}"
    local stage_dir="${STAGE_DIR:-${work_dir}/debian/${package}}"
    local result_dir="${RESULT_DIR:-_build/deb-result}"

    if [[ -f "${build_dir}/build.ninja" ]]; then
        meson setup "${build_dir}" --reconfigure --prefix=/usr --buildtype=release
    else
        meson setup "${build_dir}" --prefix=/usr --buildtype=release
    fi

    ninja -C "${build_dir}"

    if [[ "${RUN_TESTS:-0}" == "1" ]]; then
        meson test -C "${build_dir}" --print-errorlogs
    fi

    rm -rf "${work_dir}" "${result_dir}"
    mkdir -p "${stage_dir}" "${result_dir}" "${work_dir}/debian"

    DESTDIR="${source_dir}/${stage_dir}" meson install -C "${build_dir}" --no-rebuild

    local doc_dir="${stage_dir}/usr/share/doc/${package}"
    install -dm755 "${doc_dir}"
    install -m644 README.md "${doc_dir}/README.md"
    install -m644 NEWS "${doc_dir}/NEWS"
    install -m644 ChangeLog "${doc_dir}/ChangeLog"
    install -m644 COPYING "${doc_dir}/copyright"

    cat > "${work_dir}/debian/control" <<EOF
Source: ${package}
Section: x11
Priority: optional
Maintainer: Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
Standards-Version: 4.7.0

Package: ${package}
Architecture: any
Depends: \${shlibs:Depends}
Description: Minimalist VTE-based terminal emulator
 Germinal is a minimalist terminal emulator based on VTE. It relies on
 tmux for tabs and pane management, and uses GSettings for configuration.
EOF

    local installed_size
    installed_size="$(du -sk "${stage_dir}" | awk '{print $1}')"

    mkdir -p "${stage_dir}/DEBIAN"
    cat > "${stage_dir}/DEBIAN/control" <<EOF
Package: ${package}
Version: ${deb_version}
Section: x11
Priority: optional
Architecture: ${arch}
Maintainer: Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
Installed-Size: ${installed_size}
Depends: \${shlibs:Depends}
Recommends: tmux, xdg-utils
Homepage: https://github.com/Keruspe/Germinal
Description: Minimalist VTE-based terminal emulator
 Germinal is a minimalist terminal emulator based on VTE. It relies on
 tmux for tabs and pane management, and uses GSettings for configuration.
EOF

    local shlibdeps_binary="${source_dir}/${stage_dir}/usr/bin/germinal"
    if [[ "${stage_dir}" == "${work_dir}/debian/${package}" ]]; then
        shlibdeps_binary="debian/${package}/usr/bin/germinal"
    fi

    local shlibs_depends
    shlibs_depends="$(
        cd "${work_dir}"
        dpkg-shlibdeps -O "${shlibdeps_binary}" |
            sed -n 's/^shlibs:Depends=//p'
    )"

    local depends
    depends="$(merge_depends "${shlibs_depends}")"

    cat > "${stage_dir}/DEBIAN/control" <<EOF
Package: ${package}
Version: ${deb_version}
Section: x11
Priority: optional
Architecture: ${arch}
Maintainer: Marc-Antoine Perennou <Marc-Antoine@Perennou.com>
Installed-Size: ${installed_size}
Depends: ${depends}
Recommends: tmux, xdg-utils
Homepage: https://github.com/Keruspe/Germinal
Description: Minimalist VTE-based terminal emulator
 Germinal is a minimalist terminal emulator based on VTE. It relies on
 tmux for tabs and pane management, and uses GSettings for configuration.
EOF

    (
        cd "${stage_dir}"
        find usr -type f -print0 |
            sort -z |
            xargs -0 md5sum > DEBIAN/md5sums
    )

    local deb_path="${result_dir}/${package}_${deb_version}_${arch}.deb"
    dpkg-deb --root-owner-group --build "${stage_dir}" "${deb_path}"

    echo "Debian package available at ${deb_path}"
}

main "${@}"
