The Qt Quick Web Runtime
========================

The Qt Quick Web Runtime provides a JavaScript API for interpreting
QML and running Qt Quick in the browser. It is currently implemented
using Native Client (NaCl). This limits support to the Chrome browser.

Building
------------------------
Build and deploy as standard Qt for NaCl project: (use 'make install' to
copy over example files.)

    /path/to/qtbase/bin/qmake && make install && /path/to/qtbase/bin/nacldeployqt qtquickruntime.bc

This will generate the required runtime files:

    qtquickruntime.js
    qtloader.js
    qtquickruntime.pexe
    qtquickruntime.nmf
    [also index.html, which we will not use]

The files in this repository are BSD-licensed. The license of the
runtime build depends on the license of the Qt build used.

Using
------------------------
Create the runtime object, create a DOM element, and set the source:

    var runtime = new QtQuickRuntime()
    var element = runtime.createElement()
    runtime.setSource(source)

See qtquickruntime.js for further documentation.

An usage example is also provided:
    examples/example.html

Binary distribution
------------------------
The qt-quick-web-runtime-bin repo contains a LGPL-licensed binary distribution:

    msorvig.github.io/qt-quick-web-runtime-bin/

If you are a Qt commercial license holder you can build your own binary using
your commercially licensed Qt, the BSD licensed Qt for NaCl port, and this BSD
licensed runtime. 
