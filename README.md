# Package Version

This branch is for `libfprint-2-2` version `1:1.90.7-2`

*DISCLAIMER:* The `elanmoc2` driver is based of the `elanmoc` driver, which is based on a newer libfprint API version. I modified the driver source code to make it compile (in essence removed all references to yet unimplemented features). It **SHOULD** work in theory, but I have **NOT** tested this version locally.

# Installation

*NOTE:* Since this repository uses the package source code for Debian, the install instructions are for Debian based systems.

Uninstall `libfprint-2-2` if it is currently installed in your system.  
*NOTE:* This will also uninstall `fprintd`, which we will install again later.


## Install Build Dependencies

Use `mk-build-deps` to create a dummy package that depends on the same dependencies, as the build environment.  
Installing this package will pull and install all needed dependencies as automatic.  
After building, you can remove this package, and all build dependencies can then be removed with `apt autoremove`.

```
mk-build-deps libfprint-2-2
apt ./libfprint-build-deps_1.90.7-2_all.deb
```
*NOTE:* Because the source version of the package this branch is based of may not be available in the near future,
I included my locally generated `libfprint-build-deps_1.90.7-2_all.deb` in the repository.

## Build Package

```
cd libfprint-1.90.7
dpkg-buildpackage -b -uc -us

```

## Uninstall Dependencies

```
apt purge libfprint-build-deps
apt autopurge
```


## Install Package

```
apt install ./libfprint-2-2_1.90.7-2.1_amd64.deb
apt-mark hold libfprint-2-2
```
*NOTE:* We hold the package to prevent a newer version from replacing our custom one. We have to manually update this package until upstream incorporates the driver.

## Install Rest

```
apt install fprintd
apt install libpam-fprintd
```

# Usage

## Enroll Fingerprints

You can now use `fprintd-enroll` (or other means, like KDE Settings Menu) to enroll your fingerprints.

After enrolling you can verify your fingerprints with `fprintd-verify`.

## Fingerprint Prompt

Per default, even after you have enrolled your fingerprints, they will not be prompted anywhere.
In order to use them for authorization, you have to add the to the PAM authorization stack.

The correct way of doing this is to execute `pam-auth-update`, and select fingerprint in the menu.
This will modify the `/etc/pam.d/common-auth` file and include fingerprint authorization.

*NOTE:* Not all applications have integrated fingerprints prompts well, and it may not always be obvious when a fingerprint read is prompted.
e.g. the SDDM justs grays out the password field when it expects a fingerprint read.

### Manual PAM Configuration
Currently `pam-auth-update` modifies the PAM stack to first ask for fingerprint (10 seconds timeout) and then for a password.  
If you would like to first be asked for a password, and then for a fingerprint (on wrong or empty password submit) use following
`/etc/pam.d/common-auth`:
```
auth	[success=2 new_authtok_reqd=2 default=ignore]	pam_unix.so try_first_pass nullok
auth	[success=1 default=ignore]	pam_fprintd.so
auth	requisite			pam_deny.so
auth	required			pam_permit.so
```
