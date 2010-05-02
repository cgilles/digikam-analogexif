#ifndef GEARTREEVIEW_H
#define GEARTREEVIEW_H

#include <QTreeView>
#include <QMouseEvent>

class GearTreeView : public QTreeView
{
	Q_OBJECT

public:
	GearTreeView(QWidget *parent) : QTreeView(parent) { }

protected:
	virtual void mousePressEvent(QMouseEvent * event)
	{
		QModelIndex clickIndex = indexAt(event->pos());

		QTreeView::mousePressEvent(event);		

		if(!clickIndex.isValid() && (event->button() != Qt::RightButton))
			clearSelection();
	}
};

#endif // GEARTREEVIEW_H
