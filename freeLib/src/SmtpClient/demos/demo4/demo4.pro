#-------------------------------------------------
#
# Project created by QtCreator 2014-10-30T22:20:54
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = demo4
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += \
    demo4.cpp

# Location of SMTP Library
SMTP_LIBRARY_LOCATION = $$PWD/../../../build/SMTPEmail-Desktop-Debug

win32:CONFIG(release, debug|release): LIBS += -L$$SMTP_LIBRARY_LOCATION/release/ -lSMTPMime
else:win32:CONFIG(debug, debug|release): LIBS += -L$$SMTP_LIBRARY_LOCATION/debug/ -lSMTPMime
else:unix: LIBS += -L$$SMTP_LIBRARY_LOCATION -lSmtpMime

INCLUDEPATH += $$SMTP_LIBRARY_LOCATION
DEPENDPATH += $$SMTP_LIBRARY_LOCATION
