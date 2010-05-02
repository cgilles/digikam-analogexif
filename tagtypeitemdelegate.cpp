#include "tagtypeitemdelegate.h"
#include "exifitem.h"

#include <QComboBox>
#include <QStringList>

QWidget* TagTypeItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
	const QModelIndex &index) const
{
	QComboBox* combo = new QComboBox(parent);

	QStringList items;

	for(int i = ExifItem::TagString; i < ExifItem::TagTypeLast; i++)
		items << ExifItem::typeName((ExifItem::TagTypes)i);

	combo->addItems(items);
	combo->setFrame(false);

	QFont f;
	f.setBold(true);
	combo->setFont(f);

	return combo;
}

void TagTypeItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	QComboBox* combo = static_cast<QComboBox*>(editor);

	combo->setCurrentIndex(index.data(Qt::EditRole).toInt());
}

void TagTypeItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
	const QModelIndex &index) const
{
	QComboBox* combo = static_cast<QComboBox*>(editor);

	model->setData(index, combo->currentIndex());
}

void TagTypeItemDelegate::updateEditorGeometry(QWidget *editor,
	const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	editor->setGeometry(option.rect.adjusted(0, -2, 0, 2));
}
