#!/bin/sh
set -e

dch -v"$GBP_DEBIAN_VERSION" "New upstream release"
git add debian/changelog
debcommit

debian/update-udev-triggers.sh
