#include "config.h"
#include "goboard.h"
#include "gogame.h"
#include "gotools.h"

/* Create a bit array of hoshi points for a board shaped like REF.  */
bit_array calculate_hoshis (const go_board &ref)
{
	int size_x = ref.size_x ();
	int size_y = ref.size_y ();
	bit_array map (ref.bitsize ());

	int edge_dist_x = size_x > 12 ? 4 : 3;
	int edge_dist_y = size_y > 12 ? 4 : 3;
	int low_x = edge_dist_x - 1;
	int low_y = edge_dist_y - 1;
	int middle_x = size_x / 2;
	int middle_y = size_y / 2;
	int high_x = size_x - edge_dist_x;
	int high_y = size_y - edge_dist_y;
	if (size_x % 2 && size_x > 9) {
		map.set_bit (ref.bitpos (middle_x, low_y));
		map.set_bit (ref.bitpos (middle_x, high_y));
		if (size_y % 2 && size_y > 9)
			map.set_bit (ref.bitpos (middle_x, middle_y));
	}
	if (size_y % 2 && size_y > 9) {
		map.set_bit (ref.bitpos (low_x, middle_y));
		map.set_bit (ref.bitpos (high_x, middle_y));
	}

	map.set_bit (ref.bitpos (low_x, low_y));
	map.set_bit (ref.bitpos (high_x, low_y));
	map.set_bit (ref.bitpos (high_x, high_y));
	map.set_bit (ref.bitpos (low_x, high_y));
	return map;
}

go_board new_handicap_board (int size, int handicap)
{
	go_board b (size);

	if (size > 25 || size < 7)
	{
		qWarning("*** BoardHandler::setHandicap() - can't set handicap for this board size");
		return b;
	}

	int edge_dist = size > 12 ? 4 : 3;
	int low = edge_dist - 1;
	int middle = size / 2;
	int high = size - edge_dist;

	// extra:
	if (size == 19 && handicap > 9)
		switch (handicap)
		{
		case 13:
			b.set_stone_nounits (16, 16, black);
			FALLTHRU
		case 12:
			b.set_stone_nounits (2, 2, black);
			FALLTHRU
		case 11:
			b.set_stone_nounits (2, 16, black);
			FALLTHRU
		case 10:
			b.set_stone_nounits (16, 2, black);
			FALLTHRU
		default:
			handicap = 9;
			break;
		}

	switch (size % 2)
	{
	// odd board size
	case 1:
		switch (handicap)
		{
		case 9:
			b.set_stone_nounits (middle, middle, black);
			FALLTHRU
		case 8:
		case 7:
			if (handicap >= 8)
			{
				b.set_stone_nounits (middle, low, black);
				b.set_stone_nounits (middle, high, black);
			}
			else
				b.set_stone_nounits (middle, middle, black);
			FALLTHRU
		case 6:
		case 5:
			if (handicap >= 6)
			{
				b.set_stone_nounits (low, middle, black);
				b.set_stone_nounits (high, middle, black);
			}
			else
				b.set_stone_nounits (middle, middle, black);
			FALLTHRU
		case 4:
			b.set_stone_nounits (high, high, black);
			FALLTHRU
		case 3:
			b.set_stone_nounits (low, low, black);
			FALLTHRU
		case 2:
			b.set_stone_nounits (high, low, black);
			b.set_stone_nounits (low, high, black);
			break;
		case 1:
			break;
		default:
			qWarning("*** BoardHandler::setHandicap() - Invalid handicap given: %d", handicap);
		}
		break;

	// even board size
	case 0:
		switch (handicap)
		{
		case 4:
			b.set_stone_nounits (high, high, black);
			FALLTHRU
		case 3:
			b.set_stone_nounits (low, low, black);
			FALLTHRU
		case 2:
			b.set_stone_nounits (high, low, black);
			b.set_stone_nounits (low, high, black);
			break;
		case 1:
			break;

		default:
			qWarning("*** BoardHandler::setHandicap() - Invalid handicap given: %d", handicap);
		}
		break;

	default:
		qWarning("*** BoardHandler::setHandicap() - can't set handicap for this board size");

	}
	b.identify_units ();
	return b;
}

board_rect find_crop (const game_state *gs)
{
	const go_board &b = gs->get_board ();
	const bit_array *col_left = create_column_left (b.size_x (), b.size_y ());
	const bit_array *row_top = create_row_top (b.size_x (), b.size_y ());
	board_rect rect (b);
	const bit_array *visible = gs->visible_inherited ();
	if (visible != nullptr && visible->popcnt () > 0) {
		for (rect.x1 = 0; rect.x1 < b.size_x (); rect.x1++)
			if (visible->intersect_p (*col_left, rect.x1))
				break;
		for (rect.x2 = b.size_x (); rect.x2-- > 0;)
			if (visible->intersect_p (*col_left, rect.x2))
				break;
		for (rect.y1 = 0; rect.y1 < b.size_y (); rect.y1++)
			if (visible->intersect_p (*row_top, rect.y1 * b.size_x ()))
				break;
		for (rect.y2 = b.size_y (); rect.y2-- > 0;)
			if (visible->intersect_p (*row_top, rect.y2 * b.size_x ()))
				break;
		return rect;
	}
	bit_array stones = b.get_stones_w ();
	stones.ior (b.get_stones_b ());
	if (stones.popcnt () == 0)
		return rect;
	for (rect.x1 = 0; rect.x1 < b.size_x (); rect.x1++)
		if (stones.intersect_p (*col_left, rect.x1)) {
			if (rect.x1 < 5)
				rect.x1 = 0;
			else
				rect.x1 = std::max (rect.x1 - 2, 0);
			break;
		}
	for (rect.x2 = b.size_x (); rect.x2-- > 0;)
		if (stones.intersect_p (*col_left, rect.x2)) {
			if (b.size_x () - 1 - rect.x2 < 5)
				rect.x2 = b.size_x () - 1;
			else
				rect.x2 = std::min (rect.x2 + 2, b.size_x () - 1);
			break;
		}
	for (rect.y1 = 0; rect.y1 < b.size_y (); rect.y1++)
		if (stones.intersect_p (*row_top, rect.y1 * b.size_x ())) {
			if (rect.y1 < 5)
				rect.y1 = 0;
			else
				rect.y1 = std::max (rect.y1 - 2, 0);
			break;
		}
	for (rect.y2 = b.size_y (); rect.y2-- > 0;)
		if (stones.intersect_p (*row_top, rect.y2 * b.size_x ())) {
			if (b.size_y () - 1 - rect.y2 < 5)
				rect.y2 = b.size_y () - 1;
			else
				rect.y2 = std::min (rect.y2 + 2, b.size_y () - 1);
			break;
		}
	return rect;
}
