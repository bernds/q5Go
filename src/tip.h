/*
 * tip.h
 */

#ifndef TIP_H
#define TIP_H

#include <qtooltip.h>
#include <qlabel.h>
/* UNUSED
class Tip : public QToolTip
{
public:
	Tip(QWidget *parent);
	virtual ~Tip();

protected:
	void maybeTip(const QPoint &p);
};
*/
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
