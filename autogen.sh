#!/bin/sh

main() {
    autoreconf -i -Wall
    intltoolize --force --automake
}

main "${@}"
