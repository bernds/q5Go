/***************************************************************************
 *   Copyright (C) 2009 by The qGo Project                                 *
 *                                                                         *
 *   This file is part of qGo.   					   *
 *                                                                         *
 *   qGo is free software: you can redistribute it and/or modify           *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 *   or write to the Free Software Foundation, Inc.,                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <QSound>

class Sound : public QObject
{
Q_OBJECT

public:
	Sound( const QString &, QObject *parent = 0) : QObject(parent) {};
	virtual ~Sound() {};

	virtual void play() {}
};


class QSoundSound : public Sound
{
Q_OBJECT

public:
	QSoundSound( const QString& filename, QObject* parent=0);

	virtual void play();
private:
	QSound *qSound;
};

class SoundFactory
{
public:
	static Sound *newSound(const QString &filename, QObject *parent = 0);
};

#endif // _AUDIO_H_
