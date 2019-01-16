#include <QGraphicsView>

class SizeGraphicsView: public QGraphicsView
{
	Q_OBJECT

signals:
	void resized ();
protected:
	virtual void resizeEvent(QResizeEvent*) override
	{
		emit resized ();
	}
public:
	SizeGraphicsView (QWidget *parent) : QGraphicsView (parent)
	{
	}
};
