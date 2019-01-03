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
			property (std::string &i) : ident (i) { }
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
			proplist::const_iterator i;
			for (i = props.begin (); i != props.end (); i++)
				if ((*i)->ident == id) {
					if (require_value && (*i)->values.empty ())
						throw broken_sgf ();
					return *i;
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

	sgf (node *n) : nodes (n) { }
};

extern sgf *load_sgf (std::istream &);
extern game_state *sgf2board (sgf &);
extern std::shared_ptr<game_record> sgf2record (const sgf &);
extern std::string record2sgf (const game_record &);
