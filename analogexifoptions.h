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

#ifndef ANALOGEXIFOPTIONS_H
#define ANALOGEXIFOPTIONS_H

#include <QDialog>
#include <QAction>
#include <QModelIndex>
#include <QSettings>

#include "ui_analogexifoptions.h"

#include "optgeartemplatemodel.h"
#include "tagtypeitemdelegate.h"
#include "tagnameitemdelegate.h"
#include "tagselectvalsitemdelegate.h"

class AnalogExifOptions : public QDialog
{
	Q_OBJECT

public:
	AnalogExifOptions(QWidget *parent = 0);
	~AnalogExifOptions();

private:
	Ui::AnalogExifOptionsClass ui;

	OptGearTemplateModel* gearTempList;
	TagTypeItemDelegate* tagTypeEditor;
	TagNameItemDelegate* tagNameEditor;
	TagSelectValsItemDelegate* tagFormatEditor;

	// template context menu
	QList<QAction*> tempContextMenu;
	QModelIndex contextIndex;

	bool dirty;

	void setDirty(bool isDirty = true)
	{
		dirty = isDirty;
		ui.applyButton->setEnabled(isDirty);
		setWindowModified(isDirty);
	}

	QSettings settings;

	void loadOptions();
	bool saveOptions();
	bool checkOptions();

	void removeTag(QModelIndex idx);

	bool initialState_userNsGBox;
	bool initialState_createBkpCbox;
	bool initialState_etagsCboxStorageXp;
	bool initialState_etagsCboxStorageUser;

	QString originalNs;
	QString originalNsPrefix;

private slots:
	// on gear clicked
	void on_gearTypesList_itemClicked(QListWidgetItem* item);
	// on template context menu
	void on_gearTemplateView_customContextMenuRequested(const QPoint& pos);
	// on delete template tag
	void on_actionDelete_triggered(bool checked = false);
	// on add new template tag
	void on_actionAdd_new_tag_triggered(bool checked = false);
	// on data changed
	void gearTempList_dataChanged(const QModelIndex&, const QModelIndex&);
	// close button
	void on_cancelButton_clicked();
	// ok button
	void on_okButton_clicked();
	// apply button
	void on_applyButton_clicked();
	// selection changed
	void gearList_selectionChanged(const QItemSelection&, const QItemSelection&);
	// user ns schema changed
	void on_userNsPrefix_textEdited(const QString &);
	// unsupported tag
	void unsupportedTag(const QModelIndex& idx, const QString& tagName);

	// dirtying signals
	void on_userNsEdit_textEdited(const QString &);
	void on_userNsGBox_toggled(bool);
	void on_createBkpCbox_stateChanged(int);
	void on_etagsCboxStorageXp_stateChanged(int);
	void on_etagsCboxStorageUser_stateChanged(int);
};

#endif // ANALOGEXIFOPTIONS_H
