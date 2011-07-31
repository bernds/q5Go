/*
 * misc.cpp
 */

#include "misc.h"
#include <qregexp.h>
#include <qptrlist.h>


// convert rk to key for sorting;
// if integer is true, then result can be used to calculate handicap
QString rkToKey(QString txt, bool integer)
{
	QString rk = txt;
	QString keyStr;

	if (integer)
	{
		// NR
		if (rk == "NR") 
			return "0000";
		
		// BC ( IGS new rating stops at 23 k = BC )
		if (rk == "BC") 
			return "0800";

		// get number
		QString buffer = rk;
		buffer.replace(QRegExp("[pdk+?\\*\\s]"), "");
		bool ok;
		int pt = buffer.toInt(&ok);
		if (!ok)
			return "0000";

		// check for k,d,p
		if (rk.find("k") != -1)
		{
			pt = (31 - pt)*100 ;//+ ( (rk.find("+") != -1) ? 10:0) ;
		}
		else if (rk.find("d") != -1)
		{
			/* Former code replaced 
			// 7d == 1p
			if (pt > 7)
				pt = 3670 + (pt - 6)*30 + ((rk.find("+") != -1) ? 10:0);
			else
				pt = 3000 + pt*100 + ((rk.find("+") != -1) ? 10:0);
			*/

			/* New formula */
			pt = 3000 + pt*100 ;//+ ((rk.find("+") != -1) ? 10:0);
		}
		else if (rk.find("p") != -1)
		{
			
			/* Former code replaced 
			// 7d == 1p
			pt = 3670 + pt*30; 
			*/
			/* New formula : still 7d ~ 1p */
			pt = 3600 + pt*100 ;//+ ((rk.find("+") != -1) ? 10:0);
		
		}
		else
			return "0000";

		buffer = QString::number(pt).rightJustify(4, '0');
		return buffer;
	}
	else
	{
		// NR
		if (rk == "NR")
			return "nr";
		
		// check for k,d,p
		if (rk.find("k") != -1)
			keyStr = "c";
		else if (rk.find("d") != -1)
			keyStr = "b";
		else if (rk.find("p") != -1)
			keyStr = "a";
		else
			keyStr = "z";
		
		// get number
		QString buffer = rk;
		buffer.replace(QRegExp("[pdk+?\\*\\s]"), "");
		if (buffer.length() < 2)
		{
			keyStr += "0";
			
			// reverse sort order for dan/pro players
			if ((keyStr == "a0") || (keyStr == "b0"))
			{
				int i = buffer.toInt();
				i = 10 - i;
				buffer = QString::number(i);
			}
		}

		return keyStr + buffer;
	}
}

