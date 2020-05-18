/* ============================================================
 *
 * This file is a part of digiKam project
 * https://www.digikam.org
 *
 * Date        : 2020-05-18
 * Description : AnalogExif generic plugin.
 *
 * Copyright (C) 2020 by Gilles Caulier <caulier dot gilles at gmail dot com>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#include "analogexifgenericplugin.h"

// Qt includes

#include <QApplication>
#include <QMessageBox>
#include <QList>
#include <QUrl>

// digiKam includes

#include <dinfointerface.h>
#include <dmessagebox.h>

// Local includes

#include "analogexif.h"

namespace DigikamGenericAnalogExifPlugin
{

AnalogExifPlugin::AnalogExifPlugin(QObject* const parent)
    : DPluginGeneric(parent)
{
}

AnalogExifPlugin::~AnalogExifPlugin()
{
}

QString AnalogExifPlugin::name() const
{
    return QString::fromUtf8("Hello World");
}

QString AnalogExifPlugin::iid() const
{
    return QLatin1String(DPLUGIN_IID);
}

QIcon AnalogExifPlugin::icon() const
{
    return QIcon::fromTheme(QLatin1String("digikam"));
}

QString AnalogExifPlugin::description() const
{
    return QString::fromUtf8("A demo Hello World plugin");
}

QString AnalogExifPlugin::details() const
{
    return QString::fromUtf8("<p>This Generic plugin is a simple demo.</p>");
}

QList<DPluginAuthor> AnalogExifPlugin::authors() const
{
    return QList<DPluginAuthor>()
            << DPluginAuthor(QString::fromUtf8("Gilles Caulier"),
                             QString::fromUtf8("caulier dot gilles at gmail dot com"),
                             QString::fromUtf8("(C) 2020"))
    ;
}

void AnalogExifPlugin::setup(QObject* const parent)
{
    DPluginAction* const ac = new DPluginAction(parent);
    ac->setIcon(icon());
    ac->setText(QString::fromUtf8("Analog Exif..."));
    ac->setObjectName(QLatin1String("AnalogExif"));
    ac->setActionCategory(DPluginAction::GenericTool);

    connect(ac, SIGNAL(triggered(bool)),
            this, SLOT(slotAnalogExif()));

    addAction(ac);
}

void AnalogExifPlugin::slotAnalogExif()
{
    DInfoInterface* const iface = infoIface(sender());
    QList<QUrl> images          = iface->currentSelectedItems();
    QString caption             = QString::fromUtf8("List of current selected items (%1):").arg(images.count());

    if (images.isEmpty())
    {
        images  = iface->currentAlbumItems();
        caption = QString::fromUtf8("List of all items (%1):").arg(images.count());
    }

    if (!images.isEmpty())
    {
        QStringList items;

        foreach (const QUrl& url, images)
        {
            items << url.url();
        }

        AnalogExif* const dlg = new AnalogExif;
        dlg->initialize();
        dlg->show();
    }
}

} // namespace DigikamGenericAnalogExifPlugin
