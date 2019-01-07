#include <iostream>

#include "goboard.h"
#include "sgf.h"

char skip_whitespace (std::istream &in)
{
    char nextch;
    do {
	if (! in.get (nextch))
	    throw premature_eof ();
    } while (isspace (nextch));
    return nextch;
}

sgf::node *parse_gametree (std::istream &in)
{
    sgf::node *prev_node = 0, *first_node = 0;

    char nextch = skip_whitespace (in);
    for (;;) {
	if (nextch != ';')
	    break;

	sgf::node *this_node = new sgf::node ();
	if (prev_node)
	    prev_node->add_child (this_node);
	else
	    first_node = this_node;
	prev_node = this_node;

	nextch = skip_whitespace (in);
	for (;;) {
	    std::string idstr = "";
	    while (isalpha (nextch) && isalpha (nextch)) {
		/* When downloading from their web interface, IGS writes
		   properties with two uppercase and several ignored lowercase
		   letters.  Ideally we'd add warning flags to the SGF to
		   inform the user they have a broken file.  But for now,
		   silently ignore them.  */
		if (isupper (nextch))
		    idstr += nextch;
		if (! in.get (nextch))
		    throw premature_eof ();
	    }
	    if (idstr.length () == 0)
		break;

	    if (isspace (nextch))
		nextch = skip_whitespace (in);
	    if (nextch != '[')
	      throw broken_sgf ();

	    sgf::node::property *p = new sgf::node::property (idstr);
	    this_node->props.push_back (p);
	    while (nextch == '[') {
		std::string valstr = "";
		int escaped = 0;
		for (;;) {
		    if (! in.get (nextch))
			throw premature_eof ();
		    if (! escaped && nextch == ']')
			break;
		    escaped = ! escaped && nextch == '\\';
		    if (! escaped)
			valstr += nextch;
		}
		p->values.push_back (valstr);
		nextch = skip_whitespace (in);
	    }
	}
    }

    for (;;) {
	if (nextch == ')')
	    break;

	if (nextch != '(')
	    throw broken_sgf ();

	if (! prev_node)
	    throw broken_sgf ();

	sgf::node *n = parse_gametree (in);
	prev_node->add_child (n);
	nextch = skip_whitespace (in);
    }

    return first_node;
}

sgf *load_sgf (std::istream &in)
{
    char nextch;

    nextch = skip_whitespace (in);
    if (nextch != '(')
	throw broken_sgf ();
    sgf *s = new sgf (parse_gametree (in));
    return s;
}
