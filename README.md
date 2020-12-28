Emboarden
=========

This project depends on:

  LibPNG: https://github.com/glennrp/libpng
  LibPCB: https://github.com/cdkersey/libpcb

and can be thought of as a conduit from one to the other; PNG is read by
LibPNG, gets traced into a vector image and simplified, and gets output as
Gerber through LibPCB. The command line options are all about how pixels map to
points in the Gerber file, including scale and level of simplification. An
option is also provided to produce a drill file, where all polygons in the input are converted into circular holes.

Compilation
-----------

The build process uses a single makefile in GNU make format. Development is
done on the version of GCC shipping with Debian Testing, but emboarden and its dependencies are expected to build on nearly anything implementing C++.


Installation
------------

The makefile does not include an installation target. On Unix-like systems, the
"emboarden" executable can be manually copied to a location in the path, e.g.:

  $ cp emboarden /usr/local/bin

Use
---

The input to emboarden is a PNG image with white pixels representing blank
space and black pixels representing the presence of a layer. Despite the simple
2-color format, the .png file is expected to be a full RGB image. The most
common way to invoke emboarden is simply "emboarden myimage.png" where
myimage.png is any properly-formatted .png image. The default scale is one
thousandth of an inch per pixel, but this can be changed with the "-scale"
option. An exhaustive list of command line options and their parameters can be
found by running "emboarden -h".
