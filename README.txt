ASD Archiver 0.2.0 (c) 12 Nov 1998

Documentation placed in asd.exe
For info/documentation enter: asd -?

Building
--------

Borland C++ (makefile):
  make

Microsoft Visual C++ 6.0 (windows):
  build.bat
  (calls VCVARS32.BAT and builds asd.exe and unasd.exe
   into the build subdirectory)

The target platform is selected in asd.h, windows is the default.
Define __DOS__ or __UNIX__ there to build for the other platforms.

/Tobias Svensson tobiassvensson@home.se
