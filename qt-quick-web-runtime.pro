include(qt-quick-web-runtime.pri)

# copy example files to build  dir
example.path = $$OUT_PWD
example.files = \
    $$PWD/example/example.html \
    $$PWD/example/qml-mousearea-snippet.png
INSTALLS += example
