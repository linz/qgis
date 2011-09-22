First ensure that the following software is installed:

Visual Studio 2008 with SP1
Development install of OSGeo4W-LINZ
Cmake 2.8
Perl
git command line utils from gitmsys

Then open up a command prompt in this directory and run:

build VERSION PACKAGE_MUMBER

i.e

build 1.5.0 1

If a sucessful build occurs you should have a package tarball called qgis-linz-verson-package.tar.bz2 in this directory. Copy this file into the OSGeo4W-LINZ\release\qgis-linz directory and rebuild the setup.ini with regen.bat.

