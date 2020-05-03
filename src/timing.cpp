#include "timing.h"
#include "gogame.h"

move_timer::move_timer ()
{
	m_remaining = std::chrono::seconds (1);
	m_settings.system = time_system::none;
	reset ();
}

move_timer::move_timer (const time_settings &t)
{
	m_settings = t;
	reset ();
}

void move_timer::reset ()
{
	m_remaining = m_remaining_for_report = m_settings.main_time;
	m_in_main_time = true;
	if (m_settings.system == time_system::byoyomi)
		m_byo_periods = m_settings.byo_periods;
	else if (m_settings.system == time_system::canadian)
		m_canadian_stones = m_settings.canadian_stones;
	if (m_settings.main_time == std::chrono::seconds{0}) {
		m_remaining = m_remaining_for_report = m_settings.period_time;
		m_in_main_time = false;
	}
}

void move_timer::start ()
{
	if (m_settings.system == time_system::canadian && !m_in_main_time && m_canadian_stones == 0) {
		m_canadian_stones = m_settings.canadian_stones;
		m_remaining = m_settings.period_time;
	}
	m_ticking = true;
	m_move_start = std::chrono::steady_clock::now ();
	m_start_within_period = m_move_start;
}

/* SET is true if we should update the m_remaining time.  Otherwise we just
   leave it, unless it expires.
   Returns true if the player still has time.  */
bool move_timer::update (bool set)
{
	if (!m_ticking)
		return true;

	if (m_settings.system == time_system::none)
		return true;

	auto now = std::chrono::steady_clock::now ();
	std::chrono::duration<double> d = now - m_start_within_period;
	m_remaining_for_report = m_remaining - d;
	if (d < m_remaining) {
		if (set)
			m_remaining -= d;
		return true;
	}
	/* d now becomes time eaten into the overtime period already.  */
	d -= m_remaining;
	if (m_settings.system == time_system::absolute || m_settings.system == time_system::fischer)
		return false;

	m_start_within_period = now;
	m_remaining = m_remaining_for_report = m_settings.period_time - d;
	if (m_in_main_time) {
		m_in_main_time = false;
		return true;
	}
	if (m_settings.system == time_system::byoyomi) {
			if (m_byo_periods == 0)
				return false;
			m_byo_periods--;
		return true;
	}
	return false;
}

/* Return a string to display on the clock view, and optionally also set remaining time
   information in ST for COL.  */
std::string move_timer::report (game_state *st, stone_color col)
{
	int t = m_remaining_for_report.count ();
	int h = t / 3600;
	t -= h * 3600;
	int m = t / 60;
	t -= m * 60;
	std::string result;
	if (h > 0) {
		result = std::to_string (h) + ":";
		if (m < 10)
			result += "0";
	}
	result += std::to_string (m) + ":";
	if (t < 10)
		result += "0";
	result += std::to_string (t);

	if (!m_in_main_time) {
		if (m_settings.system == time_system::byoyomi)
			result += " (+" + std::to_string (m_byo_periods - 1) + ")";
		else if (m_settings.system == time_system::canadian)
			result += " / " + std::to_string (m_canadian_stones);
	}
	if (st != nullptr) {
		st->set_time_left (col, std::to_string (m_remaining_for_report.count ()));
		if (m_settings.system == time_system::canadian && !m_in_main_time)
			st->set_stones_left (col, std::to_string (m_canadian_stones));
		else
			st->set_stones_left (col, "");
	}
	return result;
}

/* Return a string suitable to pass as argument to the GTP time_left command.  */
std::string move_timer::report_gtp ()
{
	int t = m_remaining_for_report.count ();
	std::string result = std::to_string (t);

	if (m_in_main_time) {
		result += " 0";
	} else if (m_settings.system == time_system::canadian) {
		int s = m_canadian_stones;
		if (s == 0)
			s = m_settings.canadian_stones;

		result += " " + std::to_string (s);
	}
	return result;
}

bool move_timer::stop (bool move_played)
{
	bool retval = update (move_played);
	m_ticking = false;
	if (move_played) {
		if (!m_in_main_time && m_settings.system == time_system::byoyomi) {
			if (retval)
				m_remaining = m_settings.period_time;
		} else if (!m_in_main_time && m_settings.system == time_system::canadian) {
			m_canadian_stones--;
			/* If we reach 0, we update the time when restart the clock rather than here,
			   so that time reporting shows the time left at zero stones.  */
		} else if (m_settings.system == time_system::fischer)
			if (retval)
				m_remaining += m_settings.period_time;
		m_remaining_for_report = m_remaining;
	}
	return retval;
}

std::chrono::duration<double> move_timer::last_move_time ()
{
	auto now = std::chrono::steady_clock::now ();
	return now - m_move_start;
}
