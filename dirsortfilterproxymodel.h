#ifndef DIRSORTFILTERPROXYMODEL_H
#define DIRSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel >

// sorts the files/directory view
class DirSortFilterProxyModel : public QSortFilterProxyModel 
{
	Q_OBJECT

public:
	DirSortFilterProxyModel(QObject* parent) : QSortFilterProxyModel(parent) {}
	bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
};

#endif // DIRSORTFILTERPROXYMODEL_H
