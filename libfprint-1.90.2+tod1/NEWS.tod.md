### libfprint-TOD v1

- First public release
- Based on [libfprint 1.90.1](https://gitlab.freedesktop.org/libfprint/libfprint/-/releases#v1.90.1)
- Bumped TOD version to 1

### Highlights of the Drivers API changes

Both the driver and external APIs have changed, as both the verify and the identify functions now have early reporting mechanisms.

- Added API for early report of matching results or retry errors
- Verify and identification completion functions have been simplified
- Support variadic arguments in error functions
- Various re-definitions of ownership handling
- Add convenience API to change state after a timeout
- Add unit tests for all the drivers API

### Drivers required changes
As per the early report mechanism, drivers need to adapt, in particular:
 - New pkg-config dependency name is `libfprint-2-tod-1`
 - Verification and Identification API for non-image drivers has changed and drivers need to both `report` the result of the action and complete it:
   - `fpi_device_{verify,identify}_report` must inform whether a match/no-match or identification happened or report a *retry error*.
   - `fpi_device_{verify,identify}_complete` must be called once the device has completed the verification / identification process,  in case reporting device errors (not retry ones!)

You can see examples of changes needed in the [reference example driver](https://gitlab.freedesktop.org/3v1n0/libfprint-tod-example-driver/-/commit/8308f84f7d1cfd1b9ed0936c13c73b43a4a46772) or the [upstream synaptics driver](https://gitlab.freedesktop.org/libfprint/libfprint/-/merge_requests/112/diffs)
