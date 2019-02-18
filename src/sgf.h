#ifndef SGF_H
#define SGF_H

#include <string>
#include <list>
#include <exception>
#include <memory>

class premature_eof : public std::exception
{
};

class broken_sgf : public std::exception
{
};

class invalid_boardsize : public std::exception
{
};

struct sgf_errors
{
	bool played_on_stone = false;
	bool charset_error = false;
	/* For decorative things like move numbers.  */
	bool invalid_val = false;
	bool malformed_eval = false;
	bool empty_komi = false;
	bool empty_handicap = false;
	bool invalid_structure = false;
	bool any_set () const
	{
		return (played_on_stone || charset_error || invalid_val || malformed_eval
			|| empty_komi || empty_handicap || invalid_structure);
	}
};

struct sgf_figure
{
	bool present = false;
	int flags = 0;
	std::string title;
};

class sgf
{
public:
	class node {
		node *active;
		node **m_end_children;
	public:
		node *m_children = nullptr, *m_siblings = nullptr;

		class property {
		public:
			std::string ident;
			std::list<std::string> values;
			/* True if we ever looked up this property, indicating that
			   it is one the program understands.  */
			bool handled = false;

			property (std::string &i) : ident (i) { }
			property (const property &other)
				: ident (other.ident), values (other.values)
			{
			}
		};
		typedef std::list<property *>proplist;
		proplist props;

		node ()
		{
			active = nullptr;
			m_end_children = &m_children;
		}
		void set_active (node *c)
		{
			active = c;
		}
		node *get_active () const
		{
			return active;
		}
		void add_child (node *c)
		{
			*m_end_children = c;
			m_end_children = &c->m_siblings;
			if (! active)
				active = c;
		}
		property *find_property (const char *id, bool require_value = true) const
		{
			for (auto i: props)
				if (i->ident == id) {
					if (require_value && i->values.empty ())
						throw broken_sgf ();
					i->handled = true;
					return i;
				}
			return nullptr;
		}
		const std::string *find_property_val (const char *id) const
		{
			property *p = find_property (id);
			if (p == nullptr)
				return nullptr;
			auto it = p->values.begin ();
			const std::string *retval = &*it;
			++it;
			if (it != p->values.end ())
				throw broken_sgf ();
			return retval;
		}
	};

	node *nodes;
	sgf_errors errs;

	sgf (node *n, const sgf_errors &e) : nodes (n), errs (e)
	{
	}
};

/* There is pain around trying to support Unicode characters in file names
   across multiple platforms.
   QFile in Qt4 did not support them correctly on either Windows or Linux.
   std::istream did support them properly on Linux by using UTF-8.  For
   that reason, and because it's a low-level part trying to be independent
   of Qt, load_sgf used to use plain istream.  However, that still doesn't
   work on Windows.
   Qt got better with Qt5, so now we use the following adapter to make
   a QFile appear somewhat like an istream.  */

#include <QDataStream>
#include <QFile>
class IODeviceAdapter
{
	QIODevice &m_dev;
public:
	IODeviceAdapter (QIODevice &d) : m_dev (d)
	{
	}
	bool get (char &c) const
	{
		return m_dev.getChar (&c);
	}
};

extern sgf *load_sgf (const IODeviceAdapter &);

#endif
