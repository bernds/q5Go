#include <QString>
#include <QFontInfo>

class svg_builder {
	QString m_elts;
	int m_w, m_h;
public:
	svg_builder (int w, int h) : m_w (w), m_h (h)
	{
	}
	operator const QByteArray ()
	{
		QString header = QString ("<svg height=\"%1\" width=\"%2\" preserveAspectRatio=\"xMidYMid meet\" ").arg (m_h).arg (m_w);
		header += "xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n";
		return (header + m_elts + "</svg>\n").toUtf8 ();
	}
	QPixmap to_pixmap (int w, int h);
	QPixmap to_pixmap ();
	void text_at (double cx, double cy, double sidelen, int len,
		      const QString &txt, const QString &fill, const QFontInfo &fi,
		      const QString &stroke = QString ());
	void fixed_height_text_at (double cx, double cy, double sidelen,
				   const QString &txt, const QString &fill, const QFontInfo &fi,
				   bool vcenter);
	void circle_at (double cx, double cy, double r,
			const QString &fill, const QString &stroke, const QString &width = QString ());
	void square_at (double cx, double cy, double sidelen,
			const QString &fill, const QString &stroke);
	void rect (double x, double y, double w, double h,
		   const QString &fill, const QString &stroke);
	void line (double x1, double y1, double x2, double y2,
		   const QString &stroke, const QString &width);
	void triangle_at (double cx, double cy, double sidelen,
			  const QString &fill, const QString &stroke);
	void cross_at (double cx, double cy, double sidelen,
		       const QString &stroke);
};

