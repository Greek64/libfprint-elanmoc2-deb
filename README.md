This repository contains the source code of the Debian package `libfprint-2-2`, modified to add the [elanmoc2](https://gitlab.freedesktop.org/Depau/libfprint/-/tree/elanmoc2) fingerprint reader driver.

Although this fingerprint reader driver is originally developed for `04f3:0c4c` devices, it is also applicable for [`04f3:0c00`](https://gitlab.freedesktop.org/Depau/libfprint/-/merge_requests/1) devices.

Once the elanmoc2 driver has been incorporated upstream ([PR](https://gitlab.freedesktop.org/libfprint/libfprint/-/merge_requests/330)), this repository will become obsolete and will be archived.

# USAGE

This repository has branches depending on the version of the Debian source package.  
Checkout the desired branch, and follow the respective README instructions.

# MAINTAINANCE STATUS

This repository is provided as is. No responsibilities no guarantees.  
The main objective of this repository is to advertise the existence of fingerprint reader drivers for `04f3:0c4c` and `04f3:0c00`. The rest is the result of minimum effort porting of my own efforts and are to be seen as "nice to have".  
I will ***try*** to update this repository with new versions, but realistically speaking, I may never update this repository again if a personal need does not compell me to do so.  
For the above reasons I have also disabled issues, but have enabled discussions, to provide a central place for people to help each other out.

# DYI Cookbook

For completness sake I will provide the instructions that I used to create this repository:

```
#Download Source
apt source libfprint-2-2 --download-only

#Extract Source
dpkg-source --skip-patches -x <DOWNLOADED_FILE_NAME>.dsc

#Create Build Dependencies
mk-build-deps libfprint-2-2

#Build
dpkg-buildpackage -b -uc -us

#Fix Build Issues
#NOTE: If build fails, dpkg-buildpackage does not unapply the debian patches. If you want to manually unapply the paches run 'quilt pop -a'.

#Clean
dpkg-buildpackage -T clean

#After a sucessful build and clean, update .gitignore to incorporate all files missed by the existing .gitignores and clean processes

#Update Debian Changelog
dch --nmu
```

# References
https://gitlab.freedesktop.org/libfprint/libfprint/-/issues/406  
https://gitlab.freedesktop.org/libfprint/libfprint/-/merge_requests/330  
https://gitlab.freedesktop.org/Depau/libfprint/-/tree/elanmoc2  
https://gitlab.freedesktop.org/Depau/libfprint/-/merge_requests/1  
https://gitlab.freedesktop.org/geodic/libfprint/-/tree/elanmoc2  
