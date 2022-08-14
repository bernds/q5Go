#ifndef GOTOOLS_H
#define GOTOOLS_H

enum class movenums { off, vars, vars_long, full };

extern go_board new_handicap_board (int, int);
extern bit_array calculate_hoshis (const go_board &);
extern board_rect find_crop (const game_state *gs);
extern go_rules guess_rules (const game_info &);
extern QString rules_name (go_rules r);
extern int collect_moves (go_board &b, game_state *startpos, game_state *stop_pos, bool primary, bool from_one = false);
extern int collect_moves_for_figure (go_board &b, game_state *startpos);
extern int collect_moves_for_nonfigure (go_board &b, game_state *startpos, movenums);

#endif
