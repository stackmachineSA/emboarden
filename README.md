Emboarden
=========

This project depends on:

  LibPNG: https://github.com/glennrp/libpng
  LibPCB: https://github.com/cdkersey/libpcb

and can be thought of as a conduit from one to the other; PNG is read by
LibPNG, gets traced into a vector image and simplified, and gets output as
Gerber through LibPCB. The command line options are all about how pixels map to
points in the Gerber file, including scale and level of simplification. An
option is also provided to produce a drill file, where all polygons in the input are converted into circular holes with the same surface area.

Compilation
-----------

TODO

Installation
------------

TODO

Use
---

TODO
