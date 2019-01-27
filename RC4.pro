TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CFLAGS += -pedantic -Wall
LIBS += -lncurses -ltinfo -lpthread

SOURCES += main.c

