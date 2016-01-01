SOURCES += qtquickruntime.cpp
QT += quick
CONFIG += c++11
TARGET = qtquickruntime

OBJECTS_DIR = .obj
MOC_DIR = .moc

# copy example and runtime files to build dir
install_files.path = $$OUT_PWD
install_files.files = \
    $$PWD/qtquickruntime.js \
    $$PWD/example/example.html \
    $$PWD/example/qml-mousearea-snippet.png
INSTALLS += install_files
