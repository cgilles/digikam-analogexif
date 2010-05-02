#ifndef GEARTREEMODEL_H
#define GEARTREEMODEL_H

#include <QStandardItemModel>

class GearTreeModel : public QStandardItemModel
{
	Q_OBJECT

public:
	GearTreeModel(QObject *parent) : QStandardItemModel(parent), isApplicable(false)
	{
		reload();
	}

	// can user get data from the gear
	void setApplicable(bool applicable)
	{
		isApplicable = applicable;
		reset();
	}

	// selects the gear
	void setSelectedIndex(const QModelIndex &index)
	{
		selected = index;

		emit dataChanged(index, index);
	}

	// data role to get gear properties
	static const int GetExifData = Qt::UserRole + 1;

	QVariant data(const QModelIndex &item, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;

	// reloads the gear
	void reload();

	int bodyCount() const;

private:
	// can user get data from the gear
	bool isApplicable;

	// selected index
	QModelIndex selected;

};

#endif // GEARTREEMODEL_H
