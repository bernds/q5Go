//
// C++ Interface: searchpath
//
// Description: 
//
//
// Author:  <>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef SEARCHPATH_H
#define SEARCHPATH_H

#include <QFile>
#include <QDir>
#include <QObject>

class SearchPath : public QObject
{
	Q_OBJECT

	QList<QDir *> directoryList;

public:
	~SearchPath();
	QFile * findFile(QFile &);
	QDir * findDirContainingFile(QFile &);
	SearchPath& operator<<(const QString&);
	SearchPath& operator<<(QDir&);
	SearchPath& operator<<(const char*);
	SearchPath& operator<<(QStringList& list);
};

#endif
