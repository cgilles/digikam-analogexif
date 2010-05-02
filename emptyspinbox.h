#ifndef _EMPTYSPINBOX_H_
#define _EMPTYSPINBOX_H_

#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>

class EmptyQSpinBox : public QSpinBox 
{
	Q_OBJECT

public:

	EmptyQSpinBox(QWidget* parent) : QSpinBox(parent) { }

	void fixup(QString & input) const
	{
		if(input != "")
			QSpinBox::fixup(input);
	}

	bool isEmpty() const
	{
		return (text() == "");
	}

};

class EmptyQDoubleSpinBox : public QDoubleSpinBox 
{
	Q_OBJECT

public:

	EmptyQDoubleSpinBox(QWidget* parent) : QDoubleSpinBox(parent) { }

	void fixup(QString & input) const
	{
		if(input != "")
			QDoubleSpinBox::fixup(input);
	}

	bool isEmpty() const
	{
		return (text() == "");
	}

};

#endif // _EMPTYSPINBOX_H_