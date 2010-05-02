#ifndef ANALOGEXIFOPTIONS_H
#define ANALOGEXIFOPTIONS_H

#include <QDialog>
#include <QAction>
#include <QModelIndex>
#include <QSettings>

#include "ui_analogexifoptions.h"

#include "optgeartemplatemodel.h"
#include "tagtypeitemdelegate.h"

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

	void saveOptions();

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
};

#endif // ANALOGEXIFOPTIONS_H
