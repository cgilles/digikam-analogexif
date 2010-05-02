#ifndef TAGTYPEITEMDELEGATE_H
#define TAGTYPEITEMDELEGATE_H

#include <QItemDelegate>

class TagTypeItemDelegate : public QItemDelegate
{
	Q_OBJECT

public:
	TagTypeItemDelegate(QObject *parent) : QItemDelegate(parent) { }

	virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
		const QModelIndex &index) const;

	virtual void setEditorData(QWidget *editor, const QModelIndex &index) const;
	virtual void setModelData(QWidget *editor, QAbstractItemModel *model,
		const QModelIndex &index) const;

	virtual void updateEditorGeometry(QWidget *editor,
		const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // TAGTYPEITEMDELEGATE_H
