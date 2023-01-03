### libfprint-TOD - libfprint for Touch OEM Drivers

This is a light fork of libfprint to expose internal Drivers API in order to
create drivers as shared libraries.

Fork is hosted at: https://gitlab.freedesktop.org/3v1n0/libfprint/tree/tod

Packaging for this is handled via gbp, so you need to make sure that you have
added the remote:

    git remote add origin-tod https://gitlab.freedesktop.org/3v1n0/libfprint.git
    git fetch origin-tod --tags

To get the latest tod release, just use `gbp import-orig --uscan`.
