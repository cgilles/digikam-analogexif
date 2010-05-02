#ifndef GEARLISTVIEW_H
#define GEARLISTVIEW_H

#include <QListView>
#include <QMouseEvent>

class GearListView : public QListView
{
	Q_OBJECT

public:
	GearListView(QWidget *parent): QListView(parent) { }

protected:
	virtual void mousePressEvent(QMouseEvent * event)
	{
		QModelIndex clickIndex = indexAt(event->pos());

		QListView::mousePressEvent(event);		

		if(!clickIndex.isValid() && (event->button() != Qt::RightButton))
			clearSelection();
	}
	
};

#endif // GEARLISTVIEW_H
