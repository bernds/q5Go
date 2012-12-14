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
#include <Q3PtrList>

class SearchPath : public QObject
{
	Q_OBJECT

public:

	SearchPath();
	~SearchPath();
	QFile * findFile(QFile &);
	QDir * findDirContainingFile(QFile &);
	SearchPath& operator<<(const QString&);
	SearchPath& operator<<(QDir&);
	SearchPath& operator<<(const char*);
	SearchPath& operator<<(QStringList& list);

 private:
	Q3PtrList<QDir> directoryList;
};

#endif
