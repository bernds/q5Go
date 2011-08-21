/*
 * tip.h
 */

#ifndef TIP_H
#define TIP_H

#include <QLabel>

class StatusTip : public QLabel
{
	Q_OBJECT

public:
	StatusTip(QWidget *parent);
	virtual ~StatusTip();

public slots:
	void slotStatusTipCoords(int, int, int,bool);

signals:
	void clearStatusBar();
};

#endif
