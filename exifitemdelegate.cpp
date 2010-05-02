#include "exifitemdelegate.h"
#include "exiftreemodel.h"
#include "exifutils.h"
#include "emptyspinbox.h"

#include <QComboBox>
#include <QLineEdit>
#include <QRegExp>
#include <QRegExpValidator>
#include <QDateTime>
#include <QDateTimeEdit>

#include <climits>
#include <cfloat>

QWidget* ExifItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
	const QModelIndex &index) const
{
	int typeRole = index.model()->data(index, ExifTreeModel::GetTypeRole).toInt();

	switch(typeRole)
	{
	case ExifItem::TagString:
		{
			QLineEdit* edit = new QLineEdit(parent);
			edit->setFrame(false);

			return edit;
		}
		break;
	case ExifItem::TagInteger:
	case ExifItem::TagUInteger:
		{
			EmptyQSpinBox* spinBox = new EmptyQSpinBox(parent);

			if(typeRole == ExifItem::TagInteger)
				spinBox->setRange(INT_MIN, INT_MAX);
			else
				spinBox->setRange(0, INT_MAX);

			spinBox->setFrame(false);

			return spinBox;
		}
		break;
	case ExifItem::TagISO:
		{
			QComboBox* combo = new QComboBox(parent);

			QStringList isos;
			isos << "6" << "8" << "10" << "12" << "16" << "20" << "25" << "32" << "40" << "50" << "64";
			isos << "80" << "100" << "125" << "160" << "200" << "250" << "320" << "400" << "500";
			isos << "640" << "800" << "1000" << "1250" << "1600" << "2000" << "2500" << "3200";
			isos << "4000" << "5000" << "6400" << "12800" << "25600" << "51200" << "102400";

			combo->addItems(isos);
			combo->setFrame(false);
			combo->setEditable(true);
			combo->setValidator(new QIntValidator(1, INT_MAX, combo));

			return combo;
		}
		break;
	case ExifItem::TagRational:
	case ExifItem::TagURational:
		{
			EmptyQDoubleSpinBox* spinBox = new EmptyQDoubleSpinBox(parent);

			if(typeRole == ExifItem::TagRational)
				spinBox->setRange(DBL_MIN, DBL_MAX);
			else
				spinBox->setRange(0.0, DBL_MAX);

			spinBox->setFrame(false);
			
			return spinBox;
		}
		break;
	case ExifItem::TagAperture:
	case ExifItem::TagApertureAPEX:
		{
			QComboBox* combo = new QComboBox(parent);

			QStringList apertures;
			apertures << "0.5" << "0.7" << "1" << "1.1" << "1.2" << "1.4" << "1.6" << "1.7" << "1.8";
			apertures << "2" << "2.2" << "2.4" << "2.5" << "2.6" << "2.8" << "3.2" << "3.4" << "3.5" << "3.7";
			apertures << "4" << "4.4" << "4.5" << "4.8" << "5.0" << "5.2" << "5.6" << "6.2" << "6.3" << "6.7";
			apertures << "7.1" << "7.3" << "8" << "8.7" << "9" << "9.5" << "10" << "11" << "12" << "13" << "14";
			apertures << "15" << "16" << "17" << "18" << "19" << "20" << "21" << "22" << "32" << "45" << "64" << "90" << "128" << "256";


			combo->addItems(apertures);
			combo->setFrame(false);
			combo->setEditable(true);
			combo->setValidator(new QDoubleValidator(0.0, DBL_MAX, 1, combo));

			return combo;
		}
		break;

	case ExifItem::TagFraction:
		{
			QLineEdit* edit = new QLineEdit(parent);

			edit->setValidator(new QRegExpValidator(QRegExp("\\d+/\\d+"), edit));
			edit->setFrame(false);

			return edit;
		}
		break;

	case ExifItem::TagShutter:
		{
			QComboBox* combo = new QComboBox(parent);

			int exposures[] = { 8000, 4000, 3000, 2000, 1500, 1000, 750, 500,
								350, 320, 250, 200, 180, 160, 125, 100, 90,
								80, 60, 50, 45, 40, 30, 25, 20, 15, 10, 8, 6,
								4, 3, 2 };

			for(int i = 0; i < sizeof(exposures) / sizeof(int); i++)
			{
				combo->addItem(QString("1/%1").arg(exposures[i]));
			}

			combo->addItem("0.7");

			combo->addItem("1");

			combo->addItem("1.5");

			int secExposures[] = { 2, 3, 4, 6, 8, 10, 15, 20, 30 };

			for(int i = 0; i < sizeof(secExposures) / sizeof (int); i++)
			{
				combo->addItem(QString("%1").arg(secExposures[i]));
			}

			combo->setEditable(true);
			combo->setValidator(new QRegExpValidator(QRegExp("(\\d+/\\d+|\\d*\\.\\d+)"), combo));
			combo->setCompleter(0);
			combo->setFrame(false);

			return combo;
		}
		break;
	case ExifItem::TagDateTime:
		{
			QDateTimeEdit* dtEdit = new QDateTimeEdit(parent);

			dtEdit->setCalendarPopup(true);
			// dtEdit->setDisplayFormat("yyyy:MM:dd HH:mm:ss");
			dtEdit->setFrame(false);

			return dtEdit;
		}
	}

	return QItemDelegate::createEditor(parent, option, index);
}

void ExifItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	int typeRole = index.model()->data(index, ExifTreeModel::GetTypeRole).toInt();

	switch(typeRole)
	{
	case ExifItem::TagString:
		{
			QLineEdit* edit = static_cast<QLineEdit*>(editor);
			QVariant value = index.model()->data(index, Qt::EditRole);
			if(value != QVariant())
				edit->setText(value.toString());
		}
		break;
	case ExifItem::TagFraction:
		{
			QVariantList rational = index.model()->data(index, Qt::EditRole).toList();

			if(rational.count() == 2)
			{
				int first = rational.at(0).toInt();
				int second = rational.at(1).toInt();
				QLineEdit* edit = static_cast<QLineEdit*>(editor);

				edit->setText(QString("%1/%2").arg(first).arg(second));
			}
			else
				return;
		}
		break;

	case ExifItem::TagShutter:
		{
			QVariantList rational = index.model()->data(index, Qt::EditRole).toList();

			if(rational.count() == 2)
			{
				int first = rational.at(0).toInt(), second = rational.at(1).toInt();
				double shutter = (double)first/(double)second;
				QString text;
				if(shutter < 0.5)
				{
					text = QString("%1/%2").arg(first).arg(second);
				}
				else
				{
					text = QString("%1").arg((double)first/(double)second);
				}

				QComboBox* combo = static_cast<QComboBox*>(editor);

				int position = combo->findText(text);
				if(position < 0)
					combo->setEditText(text);
				else
					combo->setCurrentIndex(position);
			}
			else
				return;
		}
		break;

	case ExifItem::TagISO:
		{
			QVariant value = index.model()->data(index, Qt::EditRole);

			if(value == QVariant())
				return;

			int i = value.toInt();

			QComboBox* combo = static_cast<QComboBox*>(editor);

			int position = combo->findText(QString::number(i));
			if(position < 0)
				combo->setEditText(QString::number(i));
			else
				combo->setCurrentIndex(position);
		}
		break;

	case ExifItem::TagInteger:
	case ExifItem::TagUInteger:
		{
			EmptyQSpinBox* spinBox = static_cast<EmptyQSpinBox*>(editor);
			QVariant value = index.model()->data(index, Qt::EditRole);
			if(value != QVariant())
				spinBox->setValue(value.toInt());
		}
		break;

	case ExifItem::TagRational:
	case ExifItem::TagURational:
		{
			EmptyQDoubleSpinBox* spinBox = static_cast<EmptyQDoubleSpinBox*>(editor);
			QVariant value = index.model()->data(index, Qt::EditRole);
			if(value != QVariant())
				spinBox->setValue(value.toDouble());
		}
		break;

	case ExifItem::TagAperture:
	case ExifItem::TagApertureAPEX:
		{
			QComboBox* combo = static_cast<QComboBox*>(editor);
			QVariant value = index.model()->data(index, Qt::EditRole);

			if(value != QVariant())
			{
				QString text = QString::number(value.toDouble(), 'f', 1);

				int position = combo->findText(text);
				if(position < 0)
					combo->setEditText(text);
				else
					combo->setCurrentIndex(position);
			}
		}
		break;

	case ExifItem::TagDateTime:
		{
			QDateTimeEdit* dtEdit = static_cast<QDateTimeEdit*>(editor);
			QVariant value = index.model()->data(index, Qt::EditRole);

			if(value != QVariant())
			{
				QDateTime date = value.toDateTime();

				if(date.isValid())
					dtEdit->setDateTime(date);

			}
		}
		break;

	default:
		QItemDelegate::setEditorData(editor, index);
		break;
	}
}
void ExifItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
	const QModelIndex &index) const
{
	int typeRole = index.model()->data(index, ExifTreeModel::GetTypeRole).toInt();

	switch(typeRole)
	{
	case ExifItem::TagString:
		{
			QLineEdit* edit = static_cast<QLineEdit*>(editor);
			QString text = edit->text();

			if(text == "")
				return;

			model->setData(index, text);
		}
		break;

	case ExifItem::TagFraction:
		{
			QLineEdit* edit = static_cast<QLineEdit*>(editor);
			QString text = edit->text();

			if(text == "")
				return;

			// parse text since therer is no way of detecting whether user typed text or selected from list
			if(text.contains('/'))
			{
				// x/y format
				QStringList ratioStr = text.split('/', QString::SkipEmptyParts);

				QVariantList rational;
				// all checks should be done by validator, but it doesn't work
				if(ratioStr.size() != 2)
					return;

				bool ok = false;
				int first = ratioStr.at(0).toInt(&ok);
				if(!ok)
					return;

				int second = ratioStr.at(1).toInt(&ok);
				if(!ok)
					return;

				rational << first << second;
				model->setData(index, rational);
			}
		}
		break;

	case ExifItem::TagShutter:
		{
			QComboBox* combo = static_cast<QComboBox*>(editor);
			QString text = combo->currentText();

			if(text == "")
				return;

			// parse text since therer is no way of detecting whether user typed text or selected from list
			if(text.contains('/'))
			{
				// x/y format
				QStringList ratioStr = text.split('/', QString::SkipEmptyParts);

				QVariantList rational;
				// all checks should be done by validator, but it doesn't work
				if(ratioStr.size() != 2)
					return;

				bool ok = false;
				int first = ratioStr.at(0).toInt(&ok);
				if(!ok)
					return;

				int second = ratioStr.at(1).toInt(&ok);
				if(!ok)
					return;

				rational << first << second;
				model->setData(index, rational);
			}
			else
			{
				// seconds
				// should be validated by validator
				int first = 0, second = 0;

				bool ok = false;
				double value = text.toDouble(&ok);
				if(!ok)
					return;

				ExifUtils::doubleToFraction(text.toDouble(), &first, &second);

				QVariantList rational;
				// all checks should be done by validator
				rational << first << second;
				model->setData(index, rational);
			}

		}
		break;

	case ExifItem::TagInteger:
	case ExifItem::TagUInteger:
		{
			EmptyQSpinBox* spinBox = static_cast<EmptyQSpinBox*>(editor);

			if(!spinBox->isEmpty())
				model->setData(index, spinBox->value());
		}
		break;

	case ExifItem::TagISO:
		{
			QComboBox* combo = static_cast<QComboBox*>(editor);
			QString text = combo->currentText();

			if(text == "")
				return;

			bool ok = false;
			int value = text.toInt(&ok);

			if(ok)
				model->setData(index, value);
		}
		break;

	case ExifItem::TagRational:
	case ExifItem::TagURational:
		{
			EmptyQDoubleSpinBox* spinBox = static_cast<EmptyQDoubleSpinBox*>(editor);

			if(!spinBox->isEmpty())
				model->setData(index, spinBox->value());
		}
		break;

	case ExifItem::TagAperture:
	case ExifItem::TagApertureAPEX:
		{
			QComboBox* combo = static_cast<QComboBox*>(editor);
			QString text = combo->currentText();

			if(text == "")
				return;

			bool ok = false;
			double value = text.toDouble(&ok);

			if(ok)
				model->setData(index, value);
		}
		break;

	case ExifItem::TagDateTime:
		{
			QDateTimeEdit* dtEdit = static_cast<QDateTimeEdit*>(editor);
			QDateTime date = dtEdit->dateTime();

			if(date.isValid())
				model->setData(index, date);
		}
		break;

	default:
		QItemDelegate::setModelData(editor, model, index);
		break;
	}
}

void ExifItemDelegate::updateEditorGeometry(QWidget *editor,
	const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	editor->setGeometry(option.rect.adjusted(0, -2, 0, 2));

	//int typeRole = index.model()->data(index, ExifTreeModel::GetTypeRole).toInt();

	//if(typeRole == ExifItem::TagFraction)
	//{
	//	//editor->setGeometry(option.rect.adjusted(0, -2, 0, 2));
	//}
	//else
	//	QItemDelegate::updateEditorGeometry(editor, option, index);
}
