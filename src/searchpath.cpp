//
// C++ Implementation: searchpath
//
// Description: 
//
//
// Author:  <>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "searchpath.h"
#include <qstringlist.h>

SearchPath::SearchPath () : QObject ()
{
  directoryList = QPtrList<QDir>();
  directoryList.setAutoDelete(true);
}

SearchPath::~SearchPath ()
{
  directoryList.clear();
}

QFile * SearchPath::findFile(QFile &file)
{
  QDir *dir = findDirContainingFile(file);

  if (dir) {
    QFile * filep = new QFile(dir->absFilePath(file.name()));
    return filep;
  } else {
    return NULL;
  }
}

QDir * SearchPath::findDirContainingFile(QFile &file)
{
  QDir *dir;
  for (dir = directoryList.first(); dir; dir = directoryList.next()) {
    if (! dir->exists() )
      continue;
    if (dir->exists(file.name()))
      return dir;
  }
  return NULL;
}


SearchPath& SearchPath::operator<<(const QString& string)
{
  QDir dir(string);
  *this << dir;
  return *this;
}

SearchPath& SearchPath::operator<<(const char* string)
{
  QString qstring(string);
  *this << qstring;
  return *this;
}

SearchPath& SearchPath::operator<<(QStringList& list)
{
  for (QStringList::Iterator it = list.begin(); it != list.end(); ++it)
    {
      QString str = *it;
      *this << str;
    }
  return *this;
}

SearchPath& SearchPath::operator<<(QDir& dir)
{
  QDir * dirp = new QDir(dir);
  directoryList.append(dirp);
  return *this;
}
