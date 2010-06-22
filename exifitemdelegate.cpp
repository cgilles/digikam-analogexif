/*
	Copyright (C) 2010 C-41 Bytes <contact@c41bytes.com>

	This file is part of AnalogExif.

    AnalogExif is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    AnalogExif is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with AnalogExif.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "exifitemdelegate.h"
#include "exiftreemodel.h"
#include "exifutils.h"
#include "emptyspinbox.h"
#include "multitagvaluesdialog.h"

#include <QComboBox>
#include <QLineEdit>
#include <QRegExp>
#include <QRegExpValidator>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QPlainTextEdit>
#include <QApplication>
#include <QClipboard>

#include <climits>
#include <cfloat>
#include <cmath>

QWidget* ExifItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
	const QModelIndex &index) const
{
	ExifItem::TagFlags tagFlags = (ExifItem::TagFlags)index.data(ExifTreeModel::GetFlagsRole).toInt();
	ExifItem::TagType typeRole = (ExifItem::TagType)index.data(ExifTreeModel::GetTypeRole).toInt();

	// special care for choice fields
	if(tagFlags.testFlag(ExifItem::Choice))
	{
		QComboBox* combo = new QComboBox(parent);

		QList<QVariantList> values = ExifItem::parseEncodedChoiceList(index.data(ExifTreeModel::GetChoiceRole).toString(), typeRole, tagFlags);

		if(values != QList<QVariantList>())
		{
			// set the combo box items
			foreach(QVariantList valuePair, values)
			{
				combo->addItem(valuePair.at(0).toString(), valuePair.at(1));
			}
		}

		//combo->setFrame(false);

		return combo;
	}

	// special care for multi-value fields
	if(tagFlags.testFlag(ExifItem::Multi))
	{
		QString textFormat = index.data(ExifTreeModel::GetChoiceRole).toString();
		return new MultiTagValuesDialog(typeRole, textFormat, tagFlags, parent);
	}

	switch(typeRole)
	{
	case ExifItem::TagString:
		{
			if(tagFlags.testFlag(ExifItem::Ascii))
			{
				AsciiLineEdit* edit = new AsciiLineEdit(parent);
				return edit;
			}
			else
			{
				QLineEdit* edit = new QLineEdit(parent);
				return edit;
			}
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

			//spinBox->setFrame(false);

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
			//combo->setFrame(false);
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

			//spinBox->setFrame(false);
			spinBox->setDecimals(ExifUtils::DoublePrecision);
			
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
			//combo->setFrame(false);
			combo->setEditable(true);
			combo->setValidator(new QDoubleValidator(0.0, DBL_MAX, 1, combo));

			return combo;
		}
		break;

	case ExifItem::TagFraction:
		{
			QLineEdit* edit = new QLineEdit(parent);

			edit->setValidator(new QRegExpValidator(QRegExp("\\d+/\\d+"), edit));
			//edit->setFrame(false);

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

                        for(unsigned i = 0; i < sizeof(exposures) / sizeof(int); i++)
			{
				combo->addItem(QString("1/%1").arg(exposures[i]));
			}

			combo->addItem("0.7");

			combo->addItem("1");

			combo->addItem("1.5");

			int secExposures[] = { 2, 3, 4, 6, 8, 10, 15, 20, 30 };

                        for(unsigned i = 0; i < sizeof(secExposures) / sizeof (int); i++)
			{
				combo->addItem(QString("%1").arg(secExposures[i]));
			}

			combo->setEditable(true);
			combo->setValidator(new QRegExpValidator(QRegExp("(\\d+/\\d+|\\d*\\.\\d+)"), combo));
			combo->setCompleter(0);
			//combo->setFrame(false);

			return combo;
		}
		break;
	case ExifItem::TagDateTime:
		{
			QDateTimeEdit* dtEdit = new QDateTimeEdit(parent);

			dtEdit->setCalendarPopup(true);
			// dtEdit->setDisplayFormat("yyyy:MM:dd HH:mm:ss");
			//dtEdit->setFrame(false);

			return dtEdit;
		}
		break;
	case ExifItem::TagText:
		{
			QPlainTextEdit* tEdit = new QPlainTextEdit(parent);

			return tEdit;
		}
		break;
	case ExifItem::TagGPS:
		{
			QLineEdit* gpsEdit = new GpsLineEdit(parent);

			return gpsEdit;
		}
		break;
        default:
                break;
	}

	return QStyledItemDelegate::createEditor(parent, option, index);
}

void ExifItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	ExifItem::TagFlags tagFlags = (ExifItem::TagFlags)index.data(ExifTreeModel::GetFlagsRole).toInt();
	ExifItem::TagType typeRole = (ExifItem::TagType)index.data(ExifTreeModel::GetTypeRole).toInt();

	// special care for choice fields
	if(tagFlags.testFlag(ExifItem::Choice))
	{
		QComboBox* combo = static_cast<QComboBox*>(editor);

		QList<QVariantList> values = ExifItem::parseEncodedChoiceList(index.data(ExifTreeModel::GetChoiceRole).toString(), typeRole, tagFlags);

		if(values != QList<QVariantList>())
		{
			int selIdx = 0;
			QVariant curValue = index.data(Qt::EditRole);

			// set the combo box items
			foreach(QVariantList valuePair, values)
			{
				if(curValue == valuePair.at(1))
					break;

				selIdx++;				
			}

			// set current index
			combo->setCurrentIndex(selIdx);
		}

		return;
	}

	// special care for multi-value fields
	if(tagFlags.testFlag(ExifItem::Multi))
	{
		MultiTagValuesDialog* multiValDialog = static_cast<MultiTagValuesDialog*>(editor);

		if(multiValDialog)
			multiValDialog->setValues(index.data(Qt::EditRole).toList());

		return;
	}

	switch(typeRole)
	{
	case ExifItem::TagString:
	case ExifItem::TagGPS:
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

	case ExifItem::TagText:
		{
			QPlainTextEdit* tEdit = static_cast<QPlainTextEdit*>(editor);

			QVariant value = index.model()->data(index, Qt::EditRole);
			if(value != QVariant())
				tEdit->setPlainText(value.toString());
		}
		break;

	default:
		QStyledItemDelegate::setEditorData(editor, index);
		break;
	}
}
void ExifItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
	const QModelIndex &index) const
{
	ExifItem::TagFlags tagFlags = (ExifItem::TagFlags)index.data(ExifTreeModel::GetFlagsRole).toInt();
	ExifItem::TagType typeRole = (ExifItem::TagType)index.data(ExifTreeModel::GetTypeRole).toInt();

	// special care for choice fields
	if(tagFlags.testFlag(ExifItem::Choice))
	{
		QComboBox* combo = static_cast<QComboBox*>(editor);

		model->setData(index, combo->itemData(combo->currentIndex()));

		return;
	}

	// special care for multi-value fields
	if(tagFlags.testFlag(ExifItem::Multi))
	{
		MultiTagValuesDialog* multiValDialog = static_cast<MultiTagValuesDialog*>(editor);

		if(multiValDialog)
		{
			if(multiValDialog->result() == QDialog::Accepted)
			{
				QVariantList values = multiValDialog->getValues();
				if(values.count() == 0)
					model->setData(index, QVariant());
				else
					model->setData(index, values);
			}
		}

		return;
	}

	switch(typeRole)
	{
	case ExifItem::TagString:
		{
			QLineEdit* edit = static_cast<QLineEdit*>(editor);
			QString text = edit->text();

			if(text == "")
				model->setData(index, QVariant());
			else
			{
				model->setData(index, text);
			}

		}
		break;

	case ExifItem::TagGPS:
		{
			GpsLineEdit* edit = static_cast<GpsLineEdit*>(editor);
			QString text = edit->text();

			if((text == edit->getDefaultValue()) && !edit->isModified())
				model->setData(index, QVariant());
			else
				model->setData(index, text);
		}
		break;

	case ExifItem::TagFraction:
		{
			QLineEdit* edit = static_cast<QLineEdit*>(editor);
			QString text = edit->text();

			if(text == "")
			{
				model->setData(index, QVariant());
				return;
			}

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
			{
				model->setData(index, QVariant());
				return;
			}

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

                                ExifUtils::doubleToFraction(value, &first, &second);

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
			else
				model->setData(index, QVariant());
		}
		break;

	case ExifItem::TagISO:
		{
			QComboBox* combo = static_cast<QComboBox*>(editor);
			QString text = combo->currentText();

			if(text == "")
			{
				model->setData(index, QVariant());
				return;
			}

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
			else
				model->setData(index, QVariant());
		}
		break;

	case ExifItem::TagAperture:
	case ExifItem::TagApertureAPEX:
		{
			QComboBox* combo = static_cast<QComboBox*>(editor);
			QString text = combo->currentText();

			if(text == "")
			{
				model->setData(index, QVariant());
				return;
			}

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

			// TODO: handle empty strings

			if(date.isValid())
				model->setData(index, date);
		}
		break;

	case ExifItem::TagText:
		{
			QPlainTextEdit* tEdit = static_cast<QPlainTextEdit*>(editor);
			QString text = tEdit->toPlainText();

			if(text == "")
				model->setData(index, QVariant());
			else
				model->setData(index, text);
		}
		break;

	default:
		QStyledItemDelegate::setModelData(editor, model, index);
		break;
	}
}

void ExifItemDelegate::updateEditorGeometry(QWidget *editor,
	const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	ExifItem::TagFlags tagFlags = (ExifItem::TagFlags)index.data(ExifTreeModel::GetFlagsRole).toInt();
	ExifItem::TagType typeRole = (ExifItem::TagType)index.data(ExifTreeModel::GetTypeRole).toInt();

	// no need for resize for multi-value fields
	if(tagFlags.testFlag(ExifItem::Multi) && !tagFlags.testFlag(ExifItem::Choice))
		return;

	switch(typeRole)
	{
	case ExifItem::TagText:
		editor->setGeometry(option.rect.adjusted(0, -2, -2, option.rect.height()*4));
		break;
	default:
		editor->setGeometry(option.rect.adjusted(0, -2, 0, 2));
		break;
	}
}

bool ExifItemDelegate::eventFilter(QObject *object, QEvent *event)
{
    QWidget *editor = qobject_cast<QWidget*>(object);
    if (!editor)
        return false;
    if (event->type() == QEvent::KeyPress) {
		QKeyEvent* keyEvent = static_cast<QKeyEvent *>(event);
		switch (keyEvent->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            if (qobject_cast<QTextEdit *>(editor) || qobject_cast<QPlainTextEdit *>(editor))
			{
				// pass Shift+Enter and others down to enter newline, Enter closes the editor
				if(keyEvent->modifiers() == Qt::NoModifier)
				{
		            QMetaObject::invokeMethod(this, "_q_commitDataAndCloseEditor",
                                      Qt::QueuedConnection, Q_ARG(QWidget*, editor));
				    return true;
				}
			}
			break;
		}

		if(keyEvent == QKeySequence::Paste)
		{
			if(qobject_cast<GpsLineEdit*>(editor))
			{
				qobject_cast<GpsLineEdit*>(editor)->paste();
				return true;
			}
		}

	}

	return QStyledItemDelegate::eventFilter(object, event);
}

GpsLineEdit::GpsLineEdit(QWidget* parent) : QLineEdit(parent)
{
	// setFrame(false);
	setInputMask("#99\u00B0 99' 99.999\" #999\u00B0 99' 99.999\"");
	defaultValue = "+00\u00B0 00' 00.000\" +000\u00B0 00' 00.000\"";
	setText(defaultValue);
	setValidator(new QRegExpValidator(QRegExp("(\\+|\\-){1}\\d{2}\u00B0 \\d{2}' \\d{2}\\.\\d{3}\" (\\+|\\-){1}\\d{3}\u00B0 \\d{2}' \\d{2}\\.\\d{3}\""), this));
}

GpsLineEdit::GpsLineEdit(const QString& contents, QWidget* parent) : QLineEdit(contents, parent), defaultValue(contents)
{
	// setFrame(false);
	setInputMask("#99\u00B0 99' 99.999\" #999\u00B0 99' 99.999\"");
	setValidator(new QRegExpValidator(QRegExp("(\\+|\\-){1}\\d{2}\u00B0 \\d{2}' \\d{2}\\.\\d{3}\" (\\+|\\-){1}\\d{3}\u00B0 \\d{2}' \\d{2}\\.\\d{3}\""), this));
}

void GpsLineEdit::paste()
{
	QString result = "";

	// handle google maps pastes
    QString clip = QApplication::clipboard()->text(QClipboard::Clipboard);
    if (!clip.isEmpty() || hasSelectedText()) {
		// "+xx xx' xx.xxx" +xxx xx' xx.xxx" format
		QRegExp regEx("(\\+|\\-)?(\\d{1,2})\u00B0\\s*(\\d{1,2})'\\s*(\\d{1,2}(?:\\.\\d{1,3})?)\"\\s*\\,?\\s*(\\+|\\-)?(\\d{1,3})\u00B0\\s*(\\d{1,2})'\\s*(\\d{1,2}(?:\\.\\d{1,3})?)\"");
		QStringList caps;

		if(regEx.indexIn(clip) != -1)
		{
			caps = regEx.capturedTexts();

			if(caps.size() == 9)
			{
				// North/South
				if(regEx.cap(1) == "-")
					result += "-";
				else
					result += "+";

				result += QString("%1\u00B0 %2' %3\" ").arg(regEx.cap(2)).arg(regEx.cap(3)).arg(regEx.cap(4));

				// East/West
				if(regEx.cap(5) == "-")
					result += "-";
				else
					result += "+";

				result += QString("%1\u00B0 %2' %3\" ").arg(regEx.cap(6), 3, QChar('0')).arg(regEx.cap(7)).arg(regEx.cap(8));

				insert(result);
				return;
			}
		}

		// match "+xx.xxxxxx, +xxx.xxxxxx" format
		regEx.setPattern("(\\+|\\-)?(\\d{1,2}(?:\\.\\d{1,10})?){1}\\s*\\,?\\s*(\\+|\\-)?(\\d{1,2}(?:\\.\\d{1,10})?){1}");
		if(regEx.indexIn(clip) != -1)
		{
			caps = regEx.capturedTexts();

			if(caps.size() == 5)
			{
				// North/South
				if(regEx.cap(1) == "-")
					result += "-";
				else
					result += "+";

				// latitude
				int d, m;
				double s;
				ExifUtils::doubleToDegrees(regEx.cap(2).toDouble(), d, m, s);

				result += QString("%1\u00B0 %2' %3\" ").arg(d, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(s, 2, 'f', 3, QChar('0'));

				// East/West
				if(regEx.cap(3) == "-")
					result += "-";
				else
					result += "+";

				// latitude
				ExifUtils::doubleToDegrees(regEx.cap(4).toDouble(), d, m, s);

				result += QString("%1\u00B0 %2' %3\"").arg(d, 3, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(s, 2, 'f', 3, QChar('0'));

				insert(result);
				return;
			}
		}
    }

	QLineEdit::paste();
}
