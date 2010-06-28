# #####################################################################
# Automatically generated by qmake (2.01a) Sun Jun 20 15:20:06 2010
# #####################################################################
TEMPLATE = app
TARGET = AnalogExif
DEPENDPATH += .
INCLUDEPATH += .
CONFIG += uic \
    thread \
    release
QT += network \
    sql
UI_DIR = .
LIBS += -lexiv2 \
    -lexpat

# Input
HEADERS += aboutdialog.h \
    analogexif.h \
    analogexifoptions.h \
    autofillexpnum.h \
    checkedgeartreeview.h \
    copymetadatadialog.h \
    dirsortfilterproxymodel.h \
    editgear.h \
    editgeartagsmodel.h \
    editgeartreemodel.h \
    edittagselectvalues.h \
    emptyspinbox.h \
    exifitem.h \
    exifitemdelegate.h \
    exiftreemodel.h \
    exifutils.h \
    gearlistmodel.h \
    gearlistview.h \
    geartableview.h \
    geartreemodel.h \
    geartreeview.h \
    metadatatagcompleter.h \
    multitagvaluesdialog.h \
    onlineversionchecker.h \
    optgeartemplatemodel.h \
    optgeartemplateview.h \
    progressdialog.h \
    resource.h \
    resource1.h \
    tagnameitemdelegate.h \
    tagselectvalsitemdelegate.h \
    tagtypeitemdelegate.h \
    asciitextdialog.h \
    asciistringdialog.h \
    tagnameeditdialog.h
FORMS += analogexifoptions.ui \
    autofillexpnum.ui \
    copymetadatadialog.ui \
    edittagselectvalues.ui \
    multitagvaluesdialog.ui \
    progressdialog.ui \
    tagnameeditdialog.ui \
    aboutdialog_mac.ui \
    analogexif_mac.ui \
    editgear_mac.ui \
    asciistringdialog_mac.ui \
    asciitextdialog_mac.ui
SOURCES += aboutdialog.cpp \
    analogexif.cpp \
    analogexifoptions.cpp \
    autofillexpnum.cpp \
    copymetadatadialog.cpp \
    dirsortfilterproxymodel.cpp \
    editgear.cpp \
    editgeartagsmodel.cpp \
    editgeartreemodel.cpp \
    edittagselectvalues.cpp \
    exifitem.cpp \
    exifitemdelegate.cpp \
    exiftreemodel.cpp \
    exifutils.cpp \
    gearlistmodel.cpp \
    geartreemodel.cpp \
    main.cpp \
    metadatatagcompleter.cpp \
    multitagvaluesdialog.cpp \
    onlineversionchecker.cpp \
    optgeartemplatemodel.cpp \
    progressdialog.cpp \
    tagnameitemdelegate.cpp \
    tagselectvalsitemdelegate.cpp \
    tagtypeitemdelegate.cpp \
    asciitextdialog.cpp \
    asciistringdialog.cpp \
    tagnameeditdialog.cpp
RESOURCES += analogexif.qrc
OTHER_FILES += version.xml
