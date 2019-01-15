/*
* imagehandler.h
*/

#ifndef IMAGEDATA_H
#define IMAGEDATA_H

#include <QImage>
#include <QPixmap>

#include "defines.h"
#include <cmath>

struct WhiteDesc;

class ImageHandler
{
	double m_w_hard { 4 }, m_b_hard { 3 };
	double m_w_spec { 0.5 }, m_b_spec { 0.2 };
	double m_w_radius { 3 }, m_b_radius { 3 };
	int m_w_flat { 1 }, m_b_flat { 1 };
	QColor m_w_col {255, 255, 255, 255};
	QColor m_b_col {60, 60, 60, 255};

	bool m_clamshell;
	int m_look;

	/* We want the appearance to stay the same across resizes, so we generate
	   some random numbers in the constructor.  */
	std::vector<double> m_pregen_rnd;

public:
	ImageHandler();

	void init(int size);
	void rescale(int size);
	const QList<QPixmap> *getStonePixmaps() const { return &stonePixmaps; }
	const QList<QPixmap> *getGhostPixmaps() const { return &ghostPixmaps; }

	void set_stone_params (double w_hard, double b_hard, double w_spec, double b_spec,
			       double w_radius, double b_radius, int w_flat, int b_flat,
			       int look, bool clamshell)
	{
		m_w_hard = w_hard;
		m_b_hard = b_hard;
		m_w_spec = w_spec;
		m_b_spec = b_spec;
		m_w_radius = w_radius;
		m_b_radius = b_radius;
		m_w_flat = w_flat;
		m_b_flat = b_flat;
		m_clamshell = clamshell;
		m_look = look;
	}

	void paint_one_stone (QImage &, bool white, int size, int idx = 0);
	QColor white_color () { return m_w_col; }
	QColor black_color () { return m_b_col; }
	void set_white_color (const QColor &c) { m_w_col = c; }
	void set_black_color (const QColor &c) { m_b_col = c; }

protected:
	void scaleBoardPixmap(QPixmap *pix, int size);

private:
	void decideAppearance(WhiteDesc *desc, int size, int rnd_idx);

	void stone_params_from_settings ();
	void paint_black_stone_old (QImage &bi, int d);
	void paint_white_stone_old (QImage &wi, int d, bool clamshell, int idx = 0);
	void paintShadowStone (QImage &si, int d);
	void paint_stone_new (QImage &wi, int d, const QColor &, double, double, int, double,
			      bool clamshell, int idx = 0);
	void ghostImage(QImage *img);

	QList<QPixmap> stonePixmaps, ghostPixmaps;
};

#endif
