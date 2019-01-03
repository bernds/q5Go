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

SearchPath::~SearchPath ()
{
  for (auto it: directoryList)
    delete it;
  directoryList.clear();
}

QFile * SearchPath::findFile(QFile &file)
{
  QDir *dir = findDirContainingFile(file);

  if (dir) {
    QFile * filep = new QFile(dir->absoluteFilePath(file.fileName()));
    return filep;
  } else {
    return NULL;
  }
}

QDir * SearchPath::findDirContainingFile(QFile &file)
{
  for (auto dir: directoryList) {
    if (! dir->exists() )
      continue;
    if (dir->exists(file.fileName()))
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
  for (auto it: list)
    {
      *this << it;
    }
  return *this;
}

SearchPath& SearchPath::operator<<(QDir& dir)
{
  QDir * dirp = new QDir(dir);
  directoryList.append(dirp);
  return *this;
}
