# TOD drivers to use for testing

This directory is used by the test-suite to load and verify pre-built drivers.

For main testing this directory should contain the "fake_test_dev" driver (AKA
test-device-fake) built using the minimum libfprint TOD we want to support.

In this way the library is loaded during tests and tested for all the upstream
tests and particularly test-fpi-device.

Such binaries are compiled (for each platform) using the [libfprint TOD test
drivers](https://gitlab.freedesktop.org/3v1n0/libfprint-tod-test-drivers)
project, per each supported version.
