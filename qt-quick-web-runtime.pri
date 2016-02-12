TEMPLATE = app
TARGET = qtquickruntime

QT += quick
CONFIG += c++11

OBJECTS_DIR = .obj
MOC_DIR = .moc

SOURCES += $$PWD/qtquickruntime.cpp

# copy runtime file to build dir
runtime.path = $$OUT_PWD
runtime.files = $$PWD/qtquickruntime.js
INSTALLS += runtime
