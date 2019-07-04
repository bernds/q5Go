#ifndef TIMING_H
#define TIMING_H

#include <chrono>
#include <string>

#include "goboard.h"

class game_state;

typedef decltype (std::chrono::steady_clock::now()) ch_time;

enum class time_system { none, absolute, canadian, byoyomi, fischer };

struct time_settings {
	time_system system = time_system::none;
	std::chrono::seconds main_time {60};
	std::chrono::seconds period_time {60};
	int canadian_stones = 25;
	int byo_periods = 3;
};

class move_timer
{
	time_settings m_settings;
	union {
		int m_byo_periods;
		int m_canadian_stones;
	};
	/* Keep the time when it became the player's turn to move.
	   Used to compute the time taken after the move is played.
	   start_within_period starts out as the same time, but can
	   change if we exhaust one time period and move into another,
	   m_remaining is also adjusted in that case.  */
	ch_time m_move_start, m_start_within_period;
	/* Kept at the same value as much as possible - use
	   now () - m_start_within_period to obtain the actual remaining
	   time.  */
	std::chrono::duration<double> m_remaining;
	/* Adjusted continuously for use of the display.  */
	std::chrono::duration<double> m_remaining_for_report;
	bool m_in_main_time = true;
	bool m_ticking = false;
public:
	move_timer (const time_settings &t);
	move_timer ();
	void reset ();
	void start ();
	bool update (bool);
	bool stop (bool);
	bool ticking () const { return m_ticking; }
	const time_settings &settings () const { return m_settings; }
	std::string report (game_state * = nullptr, stone_color col = none);
	std::string report_gtp ();
	std::chrono::duration<double> last_move_time ();
};
#endif
