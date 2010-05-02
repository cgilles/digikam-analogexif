#ifndef GEARLISTMODEL_H
#define GEARLISTMODEL_H

#include <QSqlQueryModel>

class GearListModel : public QSqlQueryModel
{
	Q_OBJECT

public:
	GearListModel(QObject *parent, int gType, QString emptyMsg = QT_TR_NOOP("empty")) :
		QSqlQueryModel(parent), isApplicable(false), gearType(gType),  emptyMessage(emptyMsg) { }

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

	int rowCount(const QModelIndex &index) const;
	QVariant data(const QModelIndex &item, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;

	// reloads the gear
	void reload();

protected:
	// can user get data from the gear
	bool isApplicable;

	// gear id
	int gearType;

	// message for the empty lists
	QString emptyMessage;

	// selected index
	QModelIndex selected;
};

#endif // GEARLISTMODEL_H
