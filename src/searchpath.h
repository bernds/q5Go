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

#include <qfile.h>
#include <qdir.h>
#include <qobject.h>
#include <q3ptrlist.h>

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
