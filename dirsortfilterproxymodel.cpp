#include "dirsortfilterproxymodel.h"
#include <QFileSystemModel>

bool DirSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
	QFileSystemModel* srcModel = dynamic_cast<QFileSystemModel*>(sourceModel());

	// if compared to directory file is always less than
	if(srcModel->isDir(left) && !srcModel->isDir(right))
		return true;

	if(!srcModel->isDir(left) && srcModel->isDir(right))
		return false;

	// else compare by name
	return QString::compare(srcModel->fileName(left), srcModel->fileName(right), Qt::CaseInsensitive) < 0;
}