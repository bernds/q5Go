#include <cmath>
#include <QSvgRenderer>
#include <QPixmap>
#include <QPainter>

#include "svgbuilder.h"

void svg_builder::text_at (double cx, double cy, double sidelen, int len,
			   const QString &txt, const QString &fill, const QFontInfo &fi,
			   const QString &stroke)
{
	int real_len = txt.length ();
	if (real_len > len)
		len = real_len;
	int font_h = sidelen * 0.8 / (1 + 0.35 * (len - 1));
	/* A very crude attempt at centering vertically.  */
	int font_yoff = -font_h * 0.17;

	const QString family = fi.family() + ",sans-serif";
	const QString str = stroke.isEmpty () ? "none" : stroke;
	m_elts += "<text x=\"" + QString::number (cx);
	m_elts += "\" y=\"" + QString::number (cy + font_h / 2 + font_yoff);
	m_elts += "\" style=\"stroke:" + str + "; font-family:" + family + "; text-anchor: middle; fill: " + fill;
	if (fi.bold ())
		m_elts += "; font-weight:bold";
	m_elts += "; font-size: " + QString::number (font_h) + "px;\">";
	m_elts += txt + "</text>\n";
}

void svg_builder::fixed_height_text_at (double cx, double cy, double sidelen,
					const QString &txt, const QString &fill, const QFontInfo &fi,
					bool vcenter)
{
	int font_h = sidelen;

	const QString family = fi.family() + ",sans-serif";
	const QString str = "none";
	m_elts += "<text x=\"" + QString::number (cx);
	if (vcenter) {
		int font_h = sidelen;
		int font_yoff = -font_h * 0.17;
		cy += font_h / 2 + font_yoff;
	}
	m_elts += "\" y=\"" + QString::number (cy);
	m_elts += "\" style=\"stroke:" + str + "; font-family:" + family + "; text-anchor: middle; fill: " + fill;
	if (fi.bold ())
		m_elts += "; font-weight:bold";
	m_elts += "; font-size: " + QString::number (font_h) + "px;\">";
	m_elts += txt + "</text>\n";
}

void svg_builder::circle_at (double cx, double cy, double r,
			     const QString &fill, const QString &stroke, const QString &width)
{
	m_elts += "<circle cx=\"" + QString::number (cx);
	m_elts += "\" cy=\"" + QString::number (cy);
	m_elts += "\" r=\"" + QString::number (r);
	m_elts += "\"";
	if (stroke != "none")
		m_elts += " stroke-width=\"" + (width.isNull () ? "2" : width) + "\"";
	m_elts += " stroke=\"" + stroke + "\" fill=\"" + fill + "\" />\n";
}

void svg_builder::square_at (double cx, double cy, double sidelen, const QString &fill, const QString &stroke)
{
	m_elts += "<rect x=\"" + QString::number (cx - sidelen * 0.5);
	m_elts += "\" y=\"" + QString::number (cy - sidelen * 0.5);
	m_elts += "\" width=\"" + QString::number (sidelen);
	m_elts += "\" height=\"" + QString::number (sidelen);
	m_elts += "\"";
	if (stroke != "none")
		m_elts += " stroke-width=\"2\"";
	m_elts += " stroke=\"" + stroke + "\" fill=\"" + fill + "\" />\n";
}

void svg_builder::rect (double x, double y, double w, double h, const QString &fill, const QString &stroke)
{
	m_elts += "<rect x=\"" + QString::number (x);
	m_elts += "\" y=\"" + QString::number (y);
	m_elts += "\" width=\"" + QString::number (w);
	m_elts += "\" height=\"" + QString::number (h);
	m_elts += "\"";
	if (stroke != "none")
		m_elts += " stroke-width=\"2\"";
	m_elts += " stroke=\"" + stroke + "\" fill=\"" + fill + "\" />\n";
}

void svg_builder::line (double x1, double y1, double x2, double y2, const QString &stroke, const QString &width)
{
	m_elts += "<line x1=\"" + QString::number (x1);
	m_elts += "\" y1=\"" + QString::number (y1);
	m_elts += "\" x2=\"" + QString::number (x2);
	m_elts += "\" y2=\"" + QString::number (y2);
	m_elts += "\" stroke-width=\"" + width + "\" stroke=\"" + stroke + "\" />\n";
}

void svg_builder::triangle_at (double cx, double cy, double sidelen, const QString &fill, const QString &stroke)
{
	sidelen /= 2;
	double s = sin (M_PI * 2 / 3);
	double c = cos (M_PI * 2 / 3);

	m_elts += (QString ("<polygon points=\"%1,%2 %3,%4 %5,%6\"")
		   .arg (cx).arg (cy - sidelen)
		   .arg (cx - s * sidelen).arg (cy - c * sidelen)
		   .arg (cx + s * sidelen).arg (cy - c * sidelen));
	if (stroke != "none")
		m_elts += " stroke-width=\"2\"";
	m_elts += " stroke=\"" + stroke + "\" fill=\"" + fill + "\" />\n";
}

void svg_builder::cross_at (double cx, double cy, double sidelen, const QString &stroke)
{
	sidelen /= M_SQRT2 * 2;
	line (cx - sidelen, cy - sidelen, cx + sidelen, cy + sidelen,
	      stroke, "2");
	line (cx + sidelen, cy - sidelen, cx - sidelen, cy + sidelen,
	      stroke, "2");
}

QPixmap svg_builder::to_pixmap (int w, int h)
{
	QSvgRenderer renderer (*this);
	QPixmap img (w, h);
	img.fill (Qt::transparent);
	QPainter painter;
	painter.begin (&img);
	renderer.render (&painter);
	painter.end ();
	return img;
}

QPixmap svg_builder::to_pixmap ()
{
	return to_pixmap (m_w, m_h);
}
