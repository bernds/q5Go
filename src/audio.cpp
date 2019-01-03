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


#include "audio.h"
#include <QDir>
#include <QFile>

QSoundSound::QSoundSound(const QString &filename, QObject *parent)
	: Sound(filename, parent)
{
	qSound = new QSound(filename, this);
}

void QSoundSound::play(void)
{
	qSound->play();
}

Sound *SoundFactory::newSound(const QString &filename, QObject *parent)
{
	QFile * f;
	if (!QFile::exists(filename))
	{
		QStringList sl;
		sl << QDir::homePath() + "/ressources/sounds" 
			<< "../ressources/sounds"
			<< "../../src/ressources/sounds"
			<< "../src/ressources/sounds"
			<< ":/ressources/sounds";

		QDir::setSearchPaths("sounds", sl);

		f = new QFile("sounds:" + filename.right(filename.length() - filename.lastIndexOf('/') - 1));
	}
	else
		f = new QFile(filename);
	return new QSoundSound(f->fileName(), parent);
}
