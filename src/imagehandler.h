/*
* imagehandler.h
*/

#ifndef IMAGEDATA_H
#define IMAGEDATA_H

#include <QImage>
//Added by qt3to4:
#include <QPixmap>
#include "defines.h"
#include <cmath>
#include <cstdlib>

//#undef USE_ANTIALIAS
//#define USE_ANTIALIAS

typedef struct WhiteDesc_struct  {
	double cosTheta, sinTheta;
	double stripeWidth, xAdd;
	double stripeMul, zMul;
} WhiteDesc;


class ImageHandler
{
public:
	ImageHandler();
	~ImageHandler();

	void init(int size);
	void rescale(int size);
	static QPixmap* getBoardPixmap(QString );
	static QPixmap* getTablePixmap(QString );
	const QList<QPixmap> *getStonePixmaps() const { return &stonePixmaps; }
	const QList<QPixmap> *getGhostPixmaps() const { return &ghostPixmaps; }
	static QList<QPixmap> *getAlternateGhostPixmaps() { return altGhostPixmaps; }


protected:
	void scaleBoardPixmap(QPixmap *pix, int size);
	
private:
	void icopy(int *im, QImage &qim, int w, int h);
	void decideAppearance(WhiteDesc *desc, int size);
	double getStripe(WhiteDesc &white, double bright, double z, int x, int y);
	void paintBlackStone (QImage &bi, int d, int stone_render);
	void paintShadowStone (QImage &si, int d);
//	void paintWhiteStone2 (QImage &wi, int d, bool stripes);
	void paintWhiteStone (QImage &wi, int d, int stone_render);//bool noShadow, bool stripes);
	void ghostImage(QImage *img);

	QList<QPixmap> stonePixmaps, ghostPixmaps;
	static QList<QPixmap> *altGhostPixmaps;
	static QPixmap *tablePixmap;
	static QPixmap *woodPixmap1;//, *woodPixmap2, *woodPixmap3, *woodPixmap4, *woodPixmap5;
	static int classCounter;
//	static int antialiasingColor, ac1, ac2, ac3, ac4, ac5;
};

#endif
