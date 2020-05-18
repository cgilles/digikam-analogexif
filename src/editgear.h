/*
    Copyright (C) 2010 C-41 Bytes <contact@c41bytes.com>

    This file is part of AnalogExif.

    AnalogExif is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    AnalogExif is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with AnalogExif.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef EDITGEAR_H
#define EDITGEAR_H

#include <QDialog>
#include <QAction>
#include <QPushButton>

#ifdef Q_WS_MAC
#include "ui_editgear_mac.h"
#else
#include "ui_editgear.h"
#endif

#include "editgeartreemodel.h"
#include "exifitemdelegate.h"
#include "editgeartagsmodel.h"

class EditGear : public QDialog
{
    Q_OBJECT

public:
    EditGear(QWidget *parent = 0);
    ~EditGear();

private:
    Ui::EditGearClass ui;

    EditGearTreeModel* gearList;
    EditGearTreeModel* filmList;
    EditGearTreeModel* authorList;
    EditGearTreeModel* developerList;

    EditGearTagsModel* metadataList;
    
    // custom item editor
    ExifItemDelegate* exifItemDelegate;

    QList<QAction*> gearContextMenu, filmContextMenu, authorContextMenu, developerContextMenu;

    QModelIndex contextIndex;

    // meta tags menu
    QMenu* metaTagsMenu;
    void fillMetaTagsMenu();
    void addMetaTags(QMenu* menu, int category);

    bool dirty;

    void setDirty(bool isDirty = true)
    {
        dirty = isDirty;
        ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(isDirty);
        setWindowModified(isDirty);
    }

    void saveAndClose();

public slots:
    virtual void reject();

private slots:
    // close button
    void on_buttonBox_rejected();
    // ok button
    void on_buttonBox_accepted();
    // apply button
    void on_buttonBox_clicked(QAbstractButton* button);
    // layout changed
    void gearList_layoutChanged();
    // data changed
    void gearList_dataChanged(const QModelIndex &, const QModelIndex &);
    // gear view context menu
    void on_gearView_customContextMenuRequested(const QPoint& pos);
    // film view context menu
    void on_filmView_customContextMenuRequested(const QPoint& pos);
    // developer view context menu
    void on_developerView_customContextMenuRequested(const QPoint& pos);
    // author view context menu
    void on_authorView_customContextMenuRequested(const QPoint& pos);
    // metadata view context menu
    void on_metadataView_customContextMenuRequested(const QPoint& pos);
    // gear selected
    void gearView_clicked(const QModelIndex& index);
    // data changed
    void metadataList_dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
    void metadataList_cleared();
    // selection changed
    void gear_selectionChanged(const QItemSelection&, const QItemSelection&);
    void gear_focused();
    void film_selectionChanged(const QItemSelection&, const QItemSelection&);
    void film_focused();
    void developer_selectionChanged(const QItemSelection&, const QItemSelection&);
    void developer_focused();
    void author_selectionChanged(const QItemSelection&, const QItemSelection&);
    void author_focused();
    void metadata_selectionChanged(const QItemSelection&, const QItemSelection&);

    // add gear mini-button
    void on_addLensBtn_clicked();
    // delete mini-buttons
    void on_delGearBtn_clicked();
    void on_delFilmBtn_clicked();
    void on_delDevBtn_clicked();
    void on_delAuthorBtn_clicked();
    // duplicate mini-buttons
    void on_dupGearBtn_clicked();
    void on_dupFilmBtn_clicked();
    void on_dupDevBtn_clicked();
    void on_dupAuthorBtn_clicked();
    // add meta tag
    void on_addTagBtn_clicked();

    // menu actions
    // add new camera
    void on_actionAdd_new_camera_body_triggered(bool checked = false);
    // add new lens
    void on_actionAdd_new_camera_lens_triggered(bool checked = false);
    // add new film
    void on_actionAdd_new_film_triggered(bool checked = false);
    // add new developer
    void on_actionAdd_new_developer_triggered(bool checked = false);
    // add new author
    void on_actionAdd_new_author_triggered(bool checked = false);
    // duplicate
    void on_actionDuplicate_triggered(bool checked = false);
    // delete
    void on_actionDelete_triggered(bool checked = false);
    // delete tag
    void on_actionDelete_meta_tag_triggered(bool checked = false);
};

#endif // EDITGEAR_H
