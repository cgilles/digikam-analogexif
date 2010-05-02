#ifndef EXIFITEMDELEGATE_H
#define EXIFITEMDELEGATE_H

#include <QItemDelegate>
#include <QComboBox>

class ExifItemDelegate : public QItemDelegate
{
	Q_OBJECT

public:
	ExifItemDelegate(QObject *parent) : QItemDelegate(parent) { }

	virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
		const QModelIndex &index) const;

	virtual void setEditorData(QWidget *editor, const QModelIndex &index) const;
	virtual void setModelData(QWidget *editor, QAbstractItemModel *model,
		const QModelIndex &index) const;

	virtual void updateEditorGeometry(QWidget *editor,
		const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // EXIFITEMDELEGATE_H
