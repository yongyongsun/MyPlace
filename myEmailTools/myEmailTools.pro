#-------------------------------------------------
#
# Project created by QtCreator 2014-03-03T14:00:06
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = myEmailTools
TEMPLATE = app


SOURCES += main.cpp\
        frmmain.cpp \
    sendemailapi/emailaddress.cpp \
    sendemailapi/mimeattachment.cpp \
    sendemailapi/mimecontentformatter.cpp \
    sendemailapi/mimefile.cpp \
    sendemailapi/mimehtml.cpp \
    sendemailapi/mimeinlinefile.cpp \
    sendemailapi/mimemessage.cpp \
    sendemailapi/mimemultipart.cpp \
    sendemailapi/mimepart.cpp \
    sendemailapi/mimetext.cpp \
    sendemailapi/quotedprintable.cpp \
    sendemailapi/smtpclient.cpp \
    receivemailapi/mail.cpp \
    mailclient.cpp \
    receivemailapi/Pop3Client.cpp

HEADERS  += \
    sendemailapi/emailaddress.h \
    sendemailapi/mimeattachment.h \
    sendemailapi/mimecontentformatter.h \
    sendemailapi/mimefile.h \
    sendemailapi/mimehtml.h \
    sendemailapi/mimeinlinefile.h \
    sendemailapi/mimemessage.h \
    sendemailapi/mimemultipart.h \
    sendemailapi/mimepart.h \
    sendemailapi/mimetext.h \
    sendemailapi/quotedprintable.h \
    sendemailapi/smtpclient.h \
    frmmain.h \
    receivemailapi/mail.h \
    mailclient.h \
    receivemailapi/Pop3Client.h

FORMS    += frmmain.ui



win32:RC_FILE=main.rc

RESOURCES += \
    main.qrc
