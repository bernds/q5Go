/*
 * misc.h
 */

#ifndef MISC_H
#define MISC_H

#include <qstring.h>
/*
struct start_len
{
	int start;
	int len;
};
*/

template<class T> class Misc
{
	public:
		Misc() {};
		virtual ~Misc() {};
		// find element (0, 1, 2,...) in string
		// element del1 is third parameter,
		// optionally right delimiter (del2), else del2 = del1
		// killblanks -> only with right del. -> skip all blanks in string
		T element(const T &line, unsigned int index, const T &del1 = " ", const T &del2 = 0, bool killblanks = false) const;
//		start_len elem(const T &line, unsigned int index, const T &del1 = 0, const T &del2 = 0, bool killblanks = false) const;
};
/*
template<class T> start_len Misc<T>::elem(const T &line, unsigned int index, const T &del1, const T &del2, bool killblanks) const
{
	int len = line.length();
	int idx = index;
	int i;
	start_len result = {0,0};
	T sub;

	// kill trailing white spaces
	while ((int) line[len-1] < 33 && len > 0)
		len--;

	// right delimiter given?
	if (!del2)
	{
		// ... no right delimiter
		// element("a b c", 0, " ") -> "a"
		// element("  a b c", 0, " ") -> "a"  spaces at the beginning are skipped
		// element("a b c", 1, " ") -> "b"
		// element(" a  b  c", 1, " ") -> "b", though two spaces between "a" and "b"

		// skip delimiters at the beginning
		i = 0;
		while (i < len && line[i] == del1[0])
			i++;

		for (; idx != -1 && i < len; i++)
		{
			// skip multiple limiters before wanted sequence starts
			while (idx > 0 && line[i] == del1 && i < len-1 && line[i+1] == del1[0])
				i++;

			// look for delimiters, maybe more in series
			if (line[i] == del1[0])
				idx--;
			else if (idx == 0)
			{
				if (result.start == 0)
					result.start = i;
				result.len++;
			}
		}
	}
	else
	{
		// ... with right delimiter
		// element("a b c", 0, " ", " ") -> "b"
		// element("(a) (b c)", 0, "(", ")") -> "a"
		// element("(a) (b c)", 1, "(", ")") -> "b c"
		// element("(a) (b c)", 0, " ", ")") -> "(b c"
		// element("(a) (b c)", 1, " ", ")") -> "c"
		// element("(a) (b c)", 1, "(", "EOL") -> "b c)"

		// skip white spaces at the beginning
		i = 0;
		while (i < len && line[i] == T(" "))
			i++;

		// seek left delimiter
		idx++;
	
		for (; idx != -1 && i < len; i++)
		{
			// skip multiple limiters before wanted sequence starts
			while (idx > 0 && line[i] == del1[0] && i < len-1 && line[i+1] == del1[0])
				i++;

			if ((idx != 0 && line[i] == del1[0]) ||
			    (idx == 0 && line[i] == del2))
			{
				idx--;
			}
			else if (idx == 0)
			{
				// EOL (End Of Line)?
				if (del2 == QString("EOL"))
				{
					// copy until end of line
					for (int j = i; j < len; j++)
						if (!killblanks || line[j] != T(" "))
						{
							if (result.start == 0)
								result.start = i;
							result.len++;
						}

					// leave loop
					idx--;
				}
				else if (!killblanks || line[i] != T(" "))
				{
					if (result.start == 0)
						result.start = i;
					result.len++;
				}
			}
		}
	}
	
	return result;
}
*/
template<class T> T Misc<T>::element(const T &line, unsigned int index, const T &del1, const T &del2, bool killblanks) const
{
	int len = line.length();
	int idx = index;
	int i;
	T sub;

	// kill trailing white spaces
	while ((int) line[len-1] < 33 && len > 0)
		len--;

	// right delimiter given?
	if (!del2)
	{
		// ... no right delimiter
		// element("a b c", 0, " ") -> "a"
		// element("  a b c", 0, " ") -> "a"  spaces at the beginning are skipped
		// element("a b c", 1, " ") -> "b"
		// element(" a  b  c", 1, " ") -> "b", though two spaces between "a" and "b"

		// skip (delimiters) spaces at the beginning
		i = 0;
//		while (i < len && line[i] == del1)
		while (i < len && line[i] == T(" "))
			i++;

		for (; idx != -1 && i < len; i++)
		{
			// skip multiple (delimiters) spaces before wanted sequence starts
//			while (idx > 0 && line[i] == del1 && i < len-1 && line[i+1] == del1)
			while (idx > 0 && line[i] == T(" ") && i < len-1 && line[i+1] == T(" "))
				i++;

			// look for delimiters, maybe more in series
			if (line[i] == del1)
				idx--;
			else if (idx == 0)
				sub += line[i];
		}
	}
	else
	{
		// ... with right delimiter
		// element("a b c", 0, " ", " ") -> "b"
		// element("(a) (b c)", 0, "(", ")") -> "a"
		// element("(a) (b c)", 1, "(", ")") -> "b c"
		// element("(a) (b c)", 0, " ", ")") -> "(b c"
		// element("(a) (b c)", 1, " ", ")") -> "c"
		// element("(a) (b c)", 1, "(", "EOL") -> "b c)"

		// skip spaces at the beginning
		i = 0;
		while (i < len && line[i] == T(" "))
			i++;

		// seek left delimiter
		idx++;
	
		for (; idx != -1 && i < len; i++)
		{
			// skip multiple (delimiters) spaces before wanted sequence starts
//			while (idx > 0 && line[i] == del1 && i < len-1 && line[i+1] == del1)
			while (idx > 0 && line[i] == T(" ") && i < len-1 && line[i+1] == T(" "))
				i++;

			if ((idx != 0 && line[i] == del1) ||
			    (idx == 0 && line[i] == del2))
			{
				idx--;
			}
			else if (idx == 0)
			{
				// EOL (End Of Line)?
				if (del2 == QString("EOL"))
				{
					// copy until end of line
					for (int j = i; j < len; j++)
						if (!killblanks || line[j] != T(" "))
							sub += line[j];

					// leave loop
					idx--;
				}
				else if (!killblanks || line[i] != T(" "))
					sub += line[i];
			}
		}
	}
	
	return sub;
}


// some useful functions

QString rkToKey(QString, bool integer=false);


#endif
