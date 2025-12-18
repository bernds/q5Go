/*
*
* imagehandler.cpp
*
* the stone rendering part has been coded by Marin Ferecatu - thanks, good job!
*
*/

#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>

#include "defines.h"
#include "imagehandler.h"
#include "qglobal.h"
#include "setting.h"
#include <math.h>
#include <svgbuilder.h>

//#include <iostream>

#ifdef Q_OS_WIN
 double drand48() { return rand()*1.0/RAND_MAX; }
#endif

/**
* Stone rendering code
* Heavily inspired from Jago and CGoban code
* http://www.igoweb.org/~wms/comp/cgoban
* /http://www.rene-grothmann.de/jago/
**/

static void icopy(unsigned *im, QImage &qim, int w, int h)
{
	for (int y = 0; y < h; y++) {
		uint *p = (uint *)qim.scanLine(y);
		for(int x = 0; x < w; x++) {
			p[x] = im[y*h + x];
		}
	}
}

struct WhiteDesc {
	double cosTheta, sinTheta;
	double stripeWidth, xAdd;
	double stripeMul, zMul;
};

void ImageHandler::decideAppearance(WhiteDesc *desc, int size, int rnd_idx)
{
	double  minStripeW, maxStripeW, theta;

	minStripeW = (double)size / 20.0;
	if (minStripeW < 1.5)
		minStripeW = 1.5;
	maxStripeW = (double)size / 10.0;
	if (maxStripeW < 2.5)
		maxStripeW = 2.5;

	theta = m_pregen_rnd[rnd_idx] * 2.0 * M_PI;
	desc->cosTheta = cos(theta);
	desc->sinTheta = sin(theta);
	desc->stripeWidth = 1.5*minStripeW + (m_pregen_rnd[rnd_idx + 1] * (maxStripeW - minStripeW));

	desc->xAdd = 3*desc->stripeWidth + (double)size * 3.0;

	desc->stripeMul = 3.0;
	desc->zMul = m_pregen_rnd[rnd_idx + 2] * 650.0 + 70.0;
}


static double getStripe(WhiteDesc &white, double bright, double z, int x, int y, double range)
{
	double wBright;

	double wStripeLoc = x*white.cosTheta - y*white.sinTheta +	white.xAdd;
	double wStripeColor = fmod(wStripeLoc + (z * z * z * white.zMul) *
		white.stripeWidth,
		white.stripeWidth) / white.stripeWidth;
	wStripeColor = wStripeColor * white.stripeMul - 0.5;
	if (wStripeColor < 0.0)
		wStripeColor = -2.0 * wStripeColor;
	if (wStripeColor > 1.0)
		wStripeColor = 1.0;
	wStripeColor = wStripeColor * range + (1 - range);

	if (1 || bright < 0.95)
		wBright = bright*wStripeColor;
	else
		wBright = bright*sqrt(sqrt(wStripeColor));

	if (wBright > 1)
		wBright = 1;
	if (wBright < 0)
		wBright = 0;

	return wBright;
}
extern bool isHidingStones;
// shadow under stones
void ImageHandler::paint_shadow_stone (QImage &si, int d)
{
	//const double pixel=0.8,shadow=0.99;

	// these are the images
	unsigned *pw=new unsigned[d*d];
	int i, j,  k;
	double di, dj, d2=(double)d/2.0-5e-1, r=d2-2e-1;
	double hh;

	k=0;

	bool smallerstones = false;
	if (smallerstones) r-=1;

	for (i=0; i<d; i++)
		for (j=0; j<d; j++) {
			di=i-d2; dj=j-d2;
			hh=r-sqrt(di*di+dj*dj);
			if (hh >= 0) {
				hh = 2 * hh / r;
				hh *= m_shadow;
				if (hh > 1) hh = 1;

				pw[k]=((int)(255*hh)<<24)|(1<<16)|(1<<8)|(1);
			}
			else pw[k]=0;
			k++;

		}

	// now copy the result in QImages
	icopy(pw, si, d, d);

	// free memory
	delete[] pw;
}

static double shade_point (double x, double y, double material, double ratio, double hardness, double ambient_ratio)
{
	double light_x = -0.4;
	double light_y = 0.4;
	double light_len_xy = sqrt (light_x * light_x + light_y * light_y);
	double light_z = sqrt (1 - light_len_xy * light_len_xy);
	double center_dist = sqrt (x * x + y * y);
	double z = sqrt (1 - center_dist * center_dist);

	double dotprod = x * light_x + y * light_y + z * light_z;
#if 0
	double reflect_x = 2 * dotprod * x - light_x;
	double reflect_y = 2 * dotprod * y - light_y;
#endif
	double reflect_z = 2 * dotprod * z - light_z;
	double spdot = reflect_z;
	double specular = pow (spdot, hardness);
	double diffuse = dotprod * material;
	double s_ratio = (1 - ambient_ratio) * ratio;
	double d_ratio = (1 - ambient_ratio) * (1 - ratio);
	double ambient = ambient_ratio * material;
	double intensity = ambient + specular * s_ratio + diffuse * d_ratio;

	return intensity;
}

static double pic_radius = 0.97;

static void render (unsigned *dest, double *intense, int d, const QColor &in_col)
{
	double pix_width = 2.0 / d;
	for (int k = 0; k < d*d; k++) {
		int i = k % d;
		int j = k / d;
		double norm_x = 2.0*i / d - 1;
		double norm_y = 2.0*j / d - 1;

		double dist = sqrt(norm_x * norm_x + norm_y * norm_y);
		double alpha = 1;
		if (dist >= pic_radius - pix_width) {
			dist -= pic_radius - pix_width;
			if (dist < pix_width)
				alpha = 1 - (dist / pix_width);
			else
				alpha = 0;
		}
		QColor col = in_col;
		int h, s, v;
		col.getHsv (&h, &s, &v);
		col.setHsv (h, s, 255 * intense[k]);
		int r = 0, g = 0, b = 0;
		if (alpha > 0)
			col.getRgb (&r, &g, &b);
		dest[k] = (unsigned)(alpha * 255) * 0x01000000 + r * 0x010000 + g * 0x0100 + b;
	}
}

void ImageHandler::paint_stone_new (QImage &wi, int d, const QColor &col, double hard, double spec,
				    int flat, double radius, bool clamshell, int idx)
{
	WhiteDesc desc;
	decideAppearance(&desc, d, idx * 3);
	double f = sqrt (3);

	float d2 = (double)d/2.0-5e-1;
	float r = d2-2e-1;
	float h, s, v;
	col.getHsvF (&h, &s, &v);

	// these are the images
	double *intense = new double[d*d];
	int i, j;

	double max_int = 0;

	int k = 0;
	for (i=0; i<d; i++)
		for (j=0; j<d; j++) {
			double norm_x = 2.0*i / d - 1;
			double norm_y = 2.0*j / d - 1;

			double dist = sqrt(norm_x * norm_x + norm_y * norm_y);
			if (dist <= 1) {
				double newdist = dist;
				if (dist != 0) {
					double rdist = sqrt (dist);
					if (flat > 0)
						newdist *= rdist;
					if (flat > 1)
						newdist *= rdist;
					if (flat > 2)
						newdist *= rdist;
					if (flat > 3)
						newdist *= rdist;
					if (flat > 4)
						newdist *= rdist;
					norm_x = norm_x * (newdist / dist);
					norm_y = norm_y * (newdist / dist);
				}

				double v2 = v;

				if (clamshell) {
					double x = norm_x * d / 2;
					double y = norm_y * d / 2;
					double z = r*r-x*x-y*y;
					if (z > 0)
						z=sqrt(z)*f;
					else
						z=0;

					double xr=sqrt(6*(x*x+y*y+z*z));
					double xr1=(2*z-x+y)/xr;

					v2 = getStripe(desc, v, xr1/7.0, i, j, 0.15);
				}
				norm_x /= radius;
				norm_y /= radius;
#if 0 /* Some sort of random unevenness, but so far it doesn't look great.  */
				double rnd_x = drand48 ();
				double rnd_y = drand48 ();
				rnd_x = (rnd_x * rnd_x * rnd_x - 0.5) / 10;
				rnd_y = (rnd_y * rnd_y * rnd_y - 0.5) / 10;
				norm_x += rnd_x;
				norm_y += rnd_y;
#endif
				double intensity = shade_point (norm_x, norm_y, v2, spec, hard, m_ambient);
				max_int = std::max (intensity, max_int);
				intense[k] = intensity;
			} else
				intense[k] = 0;
			k++;
		}

	unsigned *pw = new unsigned[d*d];
	render (pw, intense, d, col);
	// now copy the result in QImages
	icopy(pw, wi, d, d);

	// free memory
	delete[] pw;
	delete[] intense;
}

void ImageHandler::paint_black_stone_old (QImage &bi, int d)
{
	const double pixel=0.8;//,shadow=0.99;

	// these are the images
	unsigned *pb=new unsigned[d*d];
	int i, j, g,g1,g2, k;
	double di, dj, d2=(double)d/2.0-5e-1, r=d2-2e-1, f=sqrt(3.0);
	double x, y, z, xr,xr1, xr2, xg1,xg2,hh;

	k=0;

	bool smallerstones = false;
	if (smallerstones) r-=1;

	for (i=0; i<d; i++)
		for (j=0; j<d; j++) {
			di=i-d2; dj=j-d2;
			hh=r-sqrt(di*di+dj*dj);
			if (hh>=0)
			{
				z=r*r-di*di-dj*dj;
				if (z>0) z=sqrt(z)*f;
				else z=0;
				x=di; y=dj;
				xr=sqrt(6*(x*x+y*y+z*z));
				xr1=(2*z-x+y)/xr;
				xr2=(2*z+x-y)/xr;

				if (xr1>0.9) xg1=(xr1-0.9)*10;
				else xg1=0;
				//if (xr2>1) xg2=(xr2-1)*10;
				if (xr2>0.96) xg2=(xr2-0.96)*10;
				else xg2=0;

				//random = drand48();

				g1=(int)(5+10*drand48() + 10*xr1 + xg1*140);
				g2=(int)(10+10* xr2+xg2*160);
				g=(g1 > g2 ? g1 : g2);
				//g=(int)1/ (1/g1 + 1/g2);

				if (hh>pixel) {
					pb[k]=(255<<24)|(g<<16)|(g<<8)|g;
				}
				else {
					pb[k]=((int)(hh/pixel*255)<<24)|(g<<16)|(g<<8)|g;
				}
			}
			else pb[k]=0;
			k++;

		}

	// now copy the result in QImages
	icopy(pb, bi, d, d);

	// free memory
	delete[] pb;
}

// shaded white stones
void ImageHandler::paint_white_stone_old (QImage &wi, int d, bool clamshell, int idx)
{
	WhiteDesc desc;
	decideAppearance(&desc, d, idx * 3);

	// the angle from which the shadow starts (measured to the light direction = pi/4)
	// alpha should be in (0, pi)
	const double ALPHA = M_PI/4;
	// how big the shadow is (should be < d/2)
	const double STRIPE = d/5.0;

	double theta;
	const double pixel=0.8;//, shadow=0.99;
	// board color
	//int col = antialiasingColor; //0xecb164;
	//int blue=col&0x0000FF,green=(col&0x00FF00)>>8,red=(col&0xFF0000)>>16;

	// these are the images
	unsigned *pw=new unsigned[d*d];
	int i, j, g, g1,g2,k;
	double di, dj, d2=(double)d/2.0-5e-1, r=d2-2e-1, f=sqrt(3.0);
	double x, y, z, xr, xr1, xr2, xg1,xg2, hh;

	k=0;

	for (i=0; i<d; i++)
		for (j=0; j<d; j++) {
			di=i-d2; dj=j-d2;
			hh=r-sqrt(di*di+dj*dj);
			if (hh>=0)
			{
				z=r*r-di*di-dj*dj;
				if (z>0) z=sqrt(z)*f;
				else z=0;
				x=di; y=dj;
				xr=sqrt(6*(x*x+y*y+z*z));
				xr1=(2*z-x+y)/xr;
				xr2=(2*z+x-y)/xr;


				if (xr1>0.9) xg1=(xr1-0.9)*10;
				else xg1=0;

				if (xr2>0.92) xg2=(xr2-0.92)*10;
				else xg2=0;

				g2=(int)(200+10* xr2+xg2*45);
				//g=(g1 > g2 ? g1 : g2);

				theta = atan2(double(j-d/2), double(i-d/2)) + M_PI - M_PI/4 + M_PI/2;
				bool stripeband = theta > ALPHA && theta < 2*M_PI-ALPHA;

				if (theta > M_PI)
					theta = (2*M_PI-theta);

				double stripe = STRIPE*sin((M_PI/2)*(theta-ALPHA)/(M_PI-ALPHA));
				if (stripe < 1) stripe = 1;

				double g1min=(int)(0+10*xr1+xg1*45), g2min=(int)(0+10*xr2+xg2*45);
				double g1max=(int)(200+10*xr1+xg1*45), g2max=(int)(200+10* xr2+xg2*45);;
				g1min = g1max - (g1max-g1min)*(1-exp(-1*(theta-ALPHA)/(M_PI-ALPHA)));
				g2min = g2max - (g2max-g2min)*(1-exp(-1*(theta-ALPHA)/(M_PI-ALPHA)));

				if (hh < STRIPE && hh > pixel && stripeband) {

					if (hh > stripe )
					{
						g1 = (int)g1max;
						g2 = (int)g2max;
					}
					else //if (hh < stripe)
					{
						g1 = int(g1min + (g1max-g1min)*sqrt(hh/stripe));
						g2 = int(g2min + (g2max-g2min)*sqrt(hh/stripe));
					}
/*						else
						{
						g1=125;
						g2=125;
						}
*/
					g=(g1 > g2 ? g1 : g2);

					if (clamshell) //stripes)
						g = 255 * getStripe(desc, g / 255., xr1/7.0, i, j, 0.15);
					pw[k]=(255<<24)|(g<<16)|((g)<<8)|(g);
				}
				else if (hh>pixel) {
					//g1=(int)(190+10*drand48()+10*xr1+xg1*45);

					g=(int)(g1max > g2max ? g1max : g2max);

					if (clamshell)
						g = 255 * getStripe(desc, g / 255., xr1/7.0, i, j, 0.15);
					pw[k]=(255<<24)|(g<<16)|((g)<<8)|(g);
				}
				else {

					g1=(int)(stripeband ? g1min : g1max);
					g2=(int)(stripeband ? g2min : g2max);

					g=(g1 > g2 ? g1 : g2);

					if (clamshell)
						g = 255 * getStripe(desc, g / 255., xr1/7.0, i, j, 0.15);

					pw[k]=((int)(hh/pixel*255)<<24)|(g<<16)|(g<<8)|g;
				}

			}
			else pw[k]=0;
			k++;
		}

	// now copy the result in QImages
	icopy(pw, wi, d, d);

	// free memory
	delete[] pw;

}

ImageHandler::ImageHandler()
{
	for (int i = 0; i < WHITE_STONES_NB * 3; i++)
		m_pregen_rnd.push_back (drand48 ());

	stone_params_from_settings ();
}

static void paint_white_stone_svg (QImage &img)
{
	/* Gets scaled to the pixmap size during render.  */
	int size = 30;
	svg_builder svg (size, size);
	double border = 1;
	svg.circle_at (size/2, size/2, size * 0.48 - border * 0.5, "white", "black", QString::number (border));
	QSvgRenderer renderer (svg);
	img.fill (Qt::transparent);
	QPainter painter;
	painter.begin (&img);
	renderer.render (&painter);
	painter.end ();
}

static void paint_black_stone_svg (QImage &img)
{
	/* Gets scaled to the pixmap size during render.  */
	int size = 30;
	svg_builder svg (size, size);
	svg.circle_at (size/2, size/2, size * 0.48, "black", "none");
	QSvgRenderer renderer (svg);
	img.fill (Qt::transparent);
	QPainter painter;
	painter.begin (&img);
	renderer.render (&painter);
	painter.end ();
}

void ImageHandler::paint_one_stone (QImage &img, bool white, int size, int idx)
{
	if (white) {
		if (m_look == 1) {
			paint_white_stone_svg (img);
		} else if (m_look == 2) {
			paint_white_stone_old (img, size, m_clamshell, idx);
		} else {
			paint_stone_new (img, size, m_w_col, m_w_hard, m_w_spec, m_w_flat, m_w_radius,
					 m_clamshell, idx);
		}
	} else {
		if (m_look == 1) {
			paint_black_stone_svg (img);
		} else if (m_look == 2) {
			paint_black_stone_old (img, size);
		} else {
			paint_stone_new (img, size, m_b_col, m_b_hard, m_b_spec, m_b_flat, m_b_radius,
					 false, 0);
		}
	}
}


void ImageHandler::init(int size)
{
	// Scale the images
	size = size * 9 / 10;

	//black stone
	QImage ib = QImage(size, size, QImage::Format_ARGB32);

	paint_one_stone (ib, false, size, 0);

	stonePixmaps.append(QPixmap::fromImage(ib,
						Qt::PreferDither |
						Qt::DiffuseAlphaDither |
						Qt::DiffuseDither));

	QImage gb(ib);
	ghostImage(&gb);
	ghostPixmaps.append(QPixmap::fromImage(gb));

	// white stones
	for (int i = 1; i <= WHITE_STONES_NB; i++)
	{
		QImage iw1(size, size, QImage::Format_ARGB32);
		paint_one_stone (iw1, true, size, i - 1);
		stonePixmaps.append(QPixmap::fromImage (iw1,
							 Qt::PreferDither |
							 Qt::DiffuseAlphaDither |
							 Qt::DiffuseDither));

		QImage gw1(iw1);
		ghostImage(&gw1);
		ghostPixmaps.append(QPixmap::fromImage(gw1));
	}

	//shadow under the stones
	QImage is = QImage(size, size, QImage::Format_ARGB32);
	if (m_look > 1)
		paint_shadow_stone(is, size);
	else
		is.fill(0);
	stonePixmaps.append(QPixmap::fromImage(is,
					       Qt::PreferDither |
					       Qt::DiffuseAlphaDither |
					       Qt::DiffuseDither));
}

void ImageHandler::rescale(int size)
{
	stone_params_from_settings ();
	size = size + 1;

	//repaint black stones
	QImage ib = QImage(size, size, QImage::Format_ARGB32);
	paint_one_stone (ib, false, size);
	stonePixmaps[0].convertFromImage(ib, Qt::PreferDither |
					 Qt::DiffuseAlphaDither |
					 Qt::DiffuseDither);
	QImage gb(ib);
	ghostImage(&gb);
	ghostPixmaps[0].convertFromImage(gb, Qt::PreferDither |
					 Qt::DiffuseAlphaDither |
					 Qt::DiffuseDither);

	// white stones
	for (int i = 1; i <= WHITE_STONES_NB; i++)
	{
		QImage iw1 = QImage(size, size, QImage::Format_ARGB32);
		paint_one_stone (iw1, true, size, i - 1);
		stonePixmaps[i].convertFromImage(iw1, Qt::PreferDither |
						 Qt::DiffuseAlphaDither |
						 Qt::DiffuseDither);
		QImage gw1(iw1);
		ghostImage(&gw1);
		ghostPixmaps[i].convertFromImage(gw1, Qt::PreferDither |
						 Qt::DiffuseAlphaDither |
						 Qt::DiffuseDither);
	}

	// shadow
	QImage is = QImage(size, size, QImage::Format_ARGB32);
	if (m_look > 1)
		paint_shadow_stone(is, size);
	else
		is.fill(0);
	stonePixmaps[WHITE_STONES_NB+1].convertFromImage(is, Qt::PreferDither |
							 Qt::DiffuseAlphaDither |
							 Qt::DiffuseDither);
#if 0
	// QQQ add transparency image
	int adding = 1+WHITE_STONES_NB; // black stone + white stone + shadow
	QImage alpha = QImage(size, size, 32);
	alpha.setAlphaBuffer(TRUE);
	alpha.reset();
	stonePixmaps->setImage(adding+1, new Q3CanvasPixmap(alpha));
	stonePixmaps->image(adding+1)->setOffset(size/2, size/2);
#endif
}

void ImageHandler::ghostImage(QImage *img)
{
	int w = img->width();
	int h = img->height();

	for (int y = 0; y < h; y++)
	{
		uint *line = (uint*)img->scanLine(y);
		for (int x = 0; x < w; x++)
		{
			//if ((x%2 && !(y%2)) || (!(x%2) && y%2))
			{
				line[x] = qRgba(qRed(line[x]), qGreen(line[x]), qBlue(line[x]), (line[x] ? 125 : 0));
			}
		}
	}
}

/* round, spec, flat, hard */
static std::vector<ImageHandler::t_params> presets =
{ { { 95, 38, 1, 16 }, { 95, 61, 1, 0 }, 15, true },
  { { 95, 35, 1, 21 }, { 95, 39, 2, 8 }, 15, true },
  { { 66, 50, 1, 47 }, { 55, 97, 1, 3 }, 27, true },
  { { 83, 85, 1, 45 }, { 80, 77, 1, 50 }, 63, false },
  { { 29, 49, 0, 36 }, { 36, 32, 0, 45 }, 28, false },
  { { 84, 85, 5, 88 }, { 95, 87, 5, 3 }, 22, false },
  { { 99, 38, 5, 20 }, { 99, 27, 5, 31 }, 15, false },
  { { 73, 50, 4, 60 }, { 66, 19, 4, 65 }, 20, true },
  { { 84, 90, 0, 29 }, { 84, 89, 0, 91 }, 75, true },
  { { 8, 90, 0, 41 }, { 8, 61, 0, 27 }, 58, true } };

void ImageHandler::set_stone_params (const t_params &used_vals, int shadow)
{
	m_b_radius = 2.05 + (100 - used_vals.b.radius) / 30.0;
	m_w_radius = 2.05 + (100 - used_vals.w.radius) / 30.0;
	m_b_spec = used_vals.b.spec / 100.0;
	m_w_spec = used_vals.w.spec / 100.0;
	m_b_flat = used_vals.b.flatten;
	m_w_flat = used_vals.w.flatten;
	m_b_hard = 1 + used_vals.b.sp_hard / 10.0;
	m_w_hard = 1 + used_vals.w.sp_hard / 10.0;
	m_ambient = used_vals.ambient / 100.0;
	m_clamshell = used_vals.clamshell;
	m_shadow = shadow / 100.0;
}

void ImageHandler::set_stone_params (int preset, int shadow)
{
	set_stone_params (presets[preset], shadow);
}

void ImageHandler::stone_params_from_settings ()
{
	int preset = setting->readIntEntry ("STONES_PRESET");
	if (preset == 0) {
		auto vals = t_params { { setting->readIntEntry ("STONES_BROUND"),
					 setting->readIntEntry ("STONES_BSPEC"),
					 setting->readIntEntry ("STONES_BFLAT"),
					 setting->readIntEntry ("STONES_BHARD") },
				       { setting->readIntEntry ("STONES_WROUND"),
					 setting->readIntEntry ("STONES_WSPEC"),
					 setting->readIntEntry ("STONES_WFLAT"),
					 setting->readIntEntry ("STONES_WHARD") },
				       setting->readIntEntry ("STONES_AMBIENT"),
				       setting->readBoolEntry ("STONES_STRIPES") };
		set_stone_params (vals, setting->readIntEntry ("STONES_SHADOWVAL"));
	} else
		set_stone_params (preset - 1, setting->readIntEntry ("STONES_SHADOWVAL"));

	m_look = setting->readIntEntry ("STONES_LOOK");

	QString wcol = setting->readEntry ("STONES_WCOL");
	QString bcol = setting->readEntry ("STONES_BCOL");
	if (wcol.isEmpty ())
		m_w_col = QColor (255, 255, 255, 255);
	else
		m_w_col = QColor (wcol);
	if (bcol.isEmpty ())
		m_b_col = QColor (60, 60, 60, 255);
	else
		m_b_col = QColor (bcol);
}
