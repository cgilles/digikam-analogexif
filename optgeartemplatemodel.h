#ifndef OPTGEARTEMPLATEMODEL_H
#define OPTGEARTEMPLATEMODEL_H

#include <QSqlQueryModel>

// needed to protect minimum working tag set
#define DEVELOPMENT_VERSION

class OptGearTemplateModel : public QSqlQueryModel
{
	Q_OBJECT

public:
	OptGearTemplateModel(QObject *parent) : QSqlQueryModel(parent) { }
	
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

	int columnCount(const QModelIndex &parent = QModelIndex()) const
	{
		// tag name, description, type and print format
#ifdef DEVELOPMENT_VERSION
		return 6;
#else
		return 5;
#endif // DEVELOPMENT_VERSION
	}

	void reload()
	{
		reload(gearId);
	}

	void reload(int id);

	// get gear that uses specified tag
	QStringList getTagUsage(const QModelIndex& idx);
	// delete tag
	void removeTag(const QModelIndex& idx);

	// insert new tag and return its id
	int insertTag(QString tagName, QString tagDesc, QString tagFormat, int tagType);

	// data role to get tag id
	static const int GetTagId = Qt::UserRole + 1;
	static const int GetTagFlagsRole = Qt::UserRole + 2;

private:
	int gearId;
};

#endif // OPTGEARTEMPLATEMODEL_H
