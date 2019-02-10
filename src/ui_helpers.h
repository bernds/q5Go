#ifndef UI_HELPERS_H
#define UI_HELPERS_H

extern QString screen_key (QWidget *);

enum class game_dialog_type { none, normal, variant };
extern std::shared_ptr<game_record> new_game_dialog (QWidget *);
extern std::shared_ptr<game_record> new_variant_game_dialog (QWidget *);
extern std::shared_ptr<game_record> record_from_stream (std::istream &isgf);
extern bool open_window_from_file (const std::string &filename);
extern void open_local_board (QWidget *, game_dialog_type);
extern go_board new_handicap_board (int, int);
extern std::string get_candidate_filename (const std::string &dir, const game_info &);
extern bit_array calculate_hoshis (const go_board &);

extern void help_about ();
extern void help_new_version ();

#endif
