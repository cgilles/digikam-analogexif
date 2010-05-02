#ifndef EDITGEARTAGSMODEL_H
#define EDITGEARTAGSMODEL_H

#include <QSqlQueryModel>

class EditGearTagsModel : public QSqlQueryModel
{
	Q_OBJECT

public:
	EditGearTagsModel(QObject *parent) : QSqlQueryModel(parent), gearId(-1) { }

	QVariant data(const QModelIndex &index, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

	int columnCount(const QModelIndex &parent = QModelIndex()) const
	{
		// tag and its value
		return 2;
	}

	void reload()
	{
		reload(gearId);
	}

	void reload(int id);

private:
	int gearId;
};

#endif // EDITGEARTAGSMODEL_H
