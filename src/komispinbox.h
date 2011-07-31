/*
* komispinbox.h
*/

#ifndef KOMISPINBOX_H
#define KOMISPINBOX_H

#include <qspinbox.h>
#include <qvalidator.h>
#include <stdlib.h>

class KomiSpinBox : public QSpinBox
{
public:
	KomiSpinBox(QWidget *parent=0, const char *name=0) : QSpinBox(parent, name)
	{
		val = new QDoubleValidator(0.0, 10.0, 1, this);
		setValidator(val);
		setMinValue(-5000); // Min -500
		setMaxValue(5000);  // Max 500
		setValue(55);       // default 5.5
		setLineStep(10);    // step 1.0
	}
	
	~KomiSpinBox()
	{
		delete val;
	}
	
	QString mapValueToText(int value)
	{
		if (value < 0 && value > -10)
			return QString("-%1.%2").arg(value/10).arg(abs(value%10));
		else
			return QString("%1.%2").arg(value/10).arg(abs(value%10));
	}
	
	int mapTextToValue(bool *ok)
	{
		if (!ok)
			qWarning("   *** Bad text value in Komi spinbox! ***");
		return int(text().toFloat()*10);
	}
	
private:
	QDoubleValidator *val;
};

#endif
