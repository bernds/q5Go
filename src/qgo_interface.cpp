/*
 *   qgo_interface.cpp - qGoClient's interface to qGo
 */

#include <QTimerEvent>
#include <QString>
#include <QTimer>
#include <QMessageBox>
#include <QComboBox>
#include <QRegExp>
#include <QGraphicsRectItem>

#include "qgo.h"

#include "mainwindow.h"
#include "qgo_interface.h"
#include "tables.h"
#include "defines.h"
#include "gs_globals.h"
#include "misc.h"
#include "config.h"
#include "clientwin.h"
#include "ui_helpers.h"
#include "gotools.h"
#include "sizegraphicsview.h"

class MainWindow_IGS : public MainWindow
{
	qGoBoard *m_connector;
	QGraphicsScene *m_preview_scene {};
	QGraphicsRectItem *m_cursor {};
	int m_preview_w = 0;
	int m_preview_h = 0;
	struct preview : public board_preview {
		qGoBoard *connector;
		bool active = true;
		preview (go_game_ptr g, game_state *st, qGoBoard *b)
			: board_preview (g, st), connector (b)
		{
		}
	};
	std::vector<preview> m_previews;
	FigureView *m_previewer {};

	QAction *m_ctab_action {};
	QAction *m_sctab_action {};

	void board_clicked (go_game_ptr game, qGoBoard *connector, game_state *st);
	void board_menu (QGraphicsSceneContextMenuEvent *e, go_game_ptr game, qGoBoard *connector, game_state *st);
	void unobserve_game (go_game_ptr);
	void update_preview (go_game_ptr game, game_state *st);
	void set_preview_cursor (const preview &);

protected:
	virtual void closeEvent (QCloseEvent *e) override;
	virtual bool comments_from_game_p () override;

public:
	MainWindow_IGS (QWidget *parent, std::shared_ptr<game_record> gr, QString screen_key,
			qGoBoard *connector, bool playing_w, bool playing_b, GameMode mode);
	~MainWindow_IGS ();

	void append_remote_comment (go_game_ptr game, const QString &s, bool colored)
	{
		if (game != m_game)
			return;
		if (colored)
			append_comment (s, setting->values.chat_color);
		else
			append_comment (s);
	}
	virtual game_state *player_move (int x, int y) override
	{
		game_state *st = MainWindow::player_move (x, y);
		if (st == nullptr || m_connector == nullptr)
			return st;
		m_connector->move_played (st, x, y);
		return st;
	}
	virtual void player_toggle_dead (int x, int y) override
	{
		if (m_connector != nullptr)
			m_connector->player_toggle_dead (x, y);
	}
	virtual void remove_connector () override;

	void playPassSound ()
	{
		if (local_stone_sound)
			qgo->playPassSound ();
	}
	void playClick ()
	{
		if (local_stone_sound)
			qgo->playStoneSound ();
	}
	void transfer_displayed (go_game_ptr game, game_state *from, game_state *to)
	{
		gfx_board->transfer_displayed (from, to);
		if (game_mode () == modeObserveMulti)
			update_preview (game, to);
	}
	void update_game_tree (go_game_ptr game)
	{
		if (game == m_game)
			MainWindow::update_game_tree ();
	}
	void resume_game (qGoBoard *connector, go_game_ptr old_game, go_game_ptr new_game, game_state *st);
	void add_game (go_game_ptr game, qGoBoard *connector, game_state *st);
	void end_game (go_game_ptr game, qGoBoard *connector, GameMode new_mode);
	void delete_last_move (go_game_ptr game, qGoBoard *connector);

	virtual void update_settings () override;
	const qGoBoard *active_board ();

public slots:
	void choices_resized ();
	virtual void slotFileClose (bool) override
	{
		if (game_mode () == modeObserveMulti)
			unobserve_game (m_game);
		else
			MainWindow::slotFileClose (false);
	}
};

static MainWindow_IGS *multi_observer_win;

MainWindow_IGS::MainWindow_IGS (QWidget *parent, std::shared_ptr<game_record> gr, QString screen_key,
				qGoBoard *brd, bool playing_w, bool playing_b, GameMode mode)
	: MainWindow (parent, gr, screen_key, mode), m_connector (brd)
{
	gfx_board->set_player_colors (playing_w, playing_b);
	/* Update clock tooltips after recording the player colors.  */
	setGameMode (mode);

	disconnect (passButton, &QPushButton::clicked, nullptr, nullptr);
	connect (passButton, &QPushButton::clicked, [this] (bool) { if (m_connector) m_connector->doPass (); });
	disconnect (undoButton, &QPushButton::clicked, nullptr, nullptr);
	connect (undoButton, &QPushButton::clicked, [this] (bool) { if (m_connector) m_connector->doUndo (); });
	disconnect (adjournButton, &QPushButton::clicked, nullptr, nullptr);
	connect (adjournButton, &QPushButton::clicked, [this] (bool) { if (m_connector) m_connector->doAdjourn (); });
	disconnect (resignButton, &QPushButton::clicked, nullptr, nullptr);
	connect (resignButton, &QPushButton::clicked, [this] (bool) { if (m_connector) m_connector->doResign (); });
	disconnect (doneButton, &QPushButton::clicked, nullptr, nullptr);
	connect (doneButton, &QPushButton::clicked, [this] (bool) { if (m_connector) m_connector->doDone (); });

	connect (this, &MainWindow::signal_sendcomment, brd, &qGoBoard::slot_sendcomment);

	// teach tools
	void (QComboBox::*cact) (const QString &) = &QComboBox::activated;
	connect (cb_opponent, cact, brd, &qGoBoard::slot_ttOpponentSelected);
	connect (pb_controls, &QPushButton::toggled, brd, &qGoBoard::slot_ttControls);
	connect (pb_mark, &QPushButton::toggled, brd, &qGoBoard::slot_ttMark);

	if (brd->ExtendedTeachingGame) {
		// make teaching features visible while observing
		cb_opponent->setDisabled (brd->IamTeacher);
		pb_controls->setDisabled (brd->IamTeacher);
		pb_mark->setDisabled (brd->IamTeacher);
	}

	if (mode == modeMatch || mode == modeTeach) {
		connect (normalTools->btimeView, &ClockView::clicked, brd, &qGoBoard::slot_addtimePauseB);
		connect (normalTools->wtimeView, &ClockView::clicked, brd, &qGoBoard::slot_addtimePauseW);
	}
	if (mode == modeObserveMulti) {
		m_previewer = new FigureView (nullptr, true);
		multi_observer_win = this;
		int w = gameChoiceView->width ();
		int h = gameChoiceView->height ();
		m_preview_scene = new QGraphicsScene (0, 0, w, h, gameChoiceView);
		gameChoiceView->setScene (m_preview_scene);
		connect (gameChoiceView, &SizeGraphicsView::resized, this, &MainWindow_IGS::choices_resized);
		connect (unobserveButton, &QPushButton::clicked, this, &MainWindow_IGS::slotFileClose);
		unobserveButton->show ();
		add_game (gr, brd, gr->get_root ());

		m_ctab_action = new QAction (this);
		m_sctab_action = new QAction (this);
		m_ctab_action->setShortcut (Qt::CTRL + Qt::Key_Tab);
		m_sctab_action->setShortcut (Qt::CTRL + Qt::SHIFT + Qt::Key_Tab);
		addAction (m_ctab_action);
		addAction (m_sctab_action);
		connect (m_ctab_action, &QAction::triggered, [this] (bool)
			 {
				 size_t len = m_previews.size ();
				 size_t i;
				 for (i = 0; i < len; i++)
					 if (m_previews[i].game == m_game)
						 break;
				 i = (i + 1) % len;
				 board_clicked (m_previews[i].game, m_previews[i].connector, m_previews[i].state);
			 });
		connect (m_sctab_action, &QAction::triggered, [this] (bool)
			 {
				 size_t len = m_previews.size ();
				 size_t i;
				 for (i = 0; i < len; i++)
					 if (m_previews[i].game == m_game)
						 break;
				 i = (i + len - 1) % len;
				 board_clicked (m_previews[i].game, m_previews[i].connector, m_previews[i].state);
			 });

	}
}

MainWindow_IGS::~MainWindow_IGS ()
{
	if (game_mode () == modeObserveMulti) {
		multi_observer_win = nullptr;
		m_previews.clear ();
	}
	delete m_previewer;
	delete m_ctab_action;
	delete m_sctab_action;
}

void MainWindow_IGS::closeEvent (QCloseEvent *e)
{
	GameMode mode = game_mode ();
	if (mode != modeObserve && mode != modeObserveMulti && !checkModified ()) {
		e->ignore ();
		return;
	}

	if (mode == modeObserveMulti) {
		for (auto &p: m_previews) {
			if (p.connector != nullptr)
				p.connector->slot_closeevent ();
		}
	} else if (m_connector != nullptr)
		m_connector->slot_closeevent ();
	e->accept ();
}

bool MainWindow_IGS::comments_from_game_p ()
{
	if (game_mode () != modeObserveMulti)
		return MainWindow::comments_from_game_p ();
	/* In the multi-observer window, we want different behaviour depending on
	   whether the game has finished or not.  */
	for (const auto &p: m_previews)
		if (p.game == m_game)
			if (p.connector == nullptr)
				return true;
	return false;
}

void MainWindow_IGS::set_preview_cursor (const preview &p)
{
	delete m_cursor;
	m_cursor = m_preview_scene->addRect (0, 0, m_preview_w, m_preview_h,
					     Qt::NoPen, QBrush (Qt::blue));
	m_cursor->setPos (p.x, p.y);
	m_cursor->setZValue (-1);
	gameChoiceView->ensureVisible (m_cursor);
}

void MainWindow_IGS::update_preview (go_game_ptr game, game_state *st)
{
	size_t i;
	size_t len = m_previews.size ();
	for (i = 0; i < len; i++) {
		if (m_previews[i].game == game)
			break;
	}
	if (i == len) {
		qWarning () << "preview not found\n";
		return;
	}
	QFontMetrics m (setting->fontStandard);
	auto info = game->info ();

	int margin = 6;
	int img_size = m_preview_w - margin;
	int font_height = m.lineSpacing () + 1;
	int img_size_y = img_size + 2 * font_height;
	auto &gmp = m_previews[i];
	gmp.state = st;
	m_previewer->reset_game (gmp.game);
	m_previewer->resizeBoard (img_size, img_size);
	m_previewer->set_show_coords (false);
	m_previewer->set_displayed (gmp.state);
	m_previewer->set_margin (0);
	QPixmap board_pm = m_previewer->draw_position (0);
	QPixmap pm (img_size, img_size_y);
	if (gmp.active) {
		pm.fill (Qt::transparent);
	} else {
		pm.fill (QColor (128, 128, 128, 255));
	}
	QPainter p;
	p.begin (&pm);
	p.setFont (setting->fontStandard);
	if (!gmp.active)
		p.setOpacity (0.5);
	p.drawImage (QPoint (0, font_height), m_previewer->background_image (), m_previewer->wood_rect ());
	p.drawPixmap (0, font_height, board_pm);
	p.setOpacity (1);
	QRect r1 (0, 0, m_preview_w - margin, font_height);
	QPen p_w = QPen (Qt::white);
	p.fillRect (r1, QBrush (Qt::black));
	p.setPen (p_w);
	p.drawText (r1, Qt::AlignVCenter, QString::fromStdString (info.name_b + " " + info.rank_b));
	QRect r2 (0, img_size + font_height, m_preview_w - margin, font_height);
	p.fillRect (r2, QBrush (Qt::white));
	QPen p_b = QPen (Qt::black);
	p.setPen (p_b);
	p.drawText (r2, Qt::AlignVCenter, QString::fromStdString (info.name_w + " " + info.rank_w));
	p.end ();
	delete gmp.pixmap;
	auto connector = gmp.connector;
	auto callback = [this, game, connector, st] () { board_clicked (game, connector, st); };
	auto menu_callback = [this, game, connector, st] (QGraphicsSceneContextMenuEvent *e) -> bool
		{
			board_menu (e, game, connector, st);
			return true;
		};
	gmp.pixmap = new ClickablePixmap (pm, callback, menu_callback);
	m_preview_scene->addItem (gmp.pixmap);
	int szdiff_x = m_preview_w - img_size;
	int szdiff_y = m_preview_h - img_size_y;
	gmp.pixmap->setPos (gmp.x + szdiff_x / 2, gmp.y + szdiff_y / 2);
	if (gmp.game == m_game)
		set_preview_cursor (gmp);
}

void MainWindow_IGS::add_game (go_game_ptr game, qGoBoard *brd, game_state *st)
{
	struct preview p (game, st, brd);
	m_previews.emplace_back (p);
	choices_resized ();
}

void MainWindow_IGS::end_game (go_game_ptr game, qGoBoard *brd, GameMode new_mode)
{
	if (game_mode () != modeObserveMulti) {
		setGameMode (new_mode);
		return;
	}
	for (auto &p: m_previews)
		if (p.game == game && p.connector == brd) {
			p.active = false;
			p.connector = nullptr;
			update_preview (game, p.state);
		}

}

const qGoBoard *MainWindow_IGS::active_board ()
{
	if (game_mode () != modeObserveMulti)
		return m_connector;
	for (auto &p: m_previews)
		if (p.game == m_game)
			return p.connector;
	return nullptr;
}

std::pair<int, int> layout_previews (QGraphicsView *view, const std::vector<board_preview *> &previews, int extra_height)
{
	int w = view->width ();
	int h = view->height ();

	int n_elts = previews.size ();

	int min_size = 220;
	int min_w = min_size;
	int min_h = min_size;
	min_h += extra_height;

	int sb_size = qApp->style()->pixelMetric (QStyle::PM_ScrollBarExtent);

	int smaller = w < h ? w : h;
	int smaller_min = w < h ? min_w : min_h;
	int larger = w < h ? h : w;
	int larger_min = w < h ? min_h : min_w;

	int new_width = 0;
	int new_height = 0;

	/* Two attempts: try to fit everything without scrollbars first, then reduce the
	   available area to account for the scroll bar.  */
	for (int assume_sb = 0; assume_sb < 2; assume_sb++) {
		/* The number of minimally-sized boards we can fit into the longer side.  */
		int n_larger_max = std::max (1, larger / larger_min);
		/* The number of minimally-sized boards we can fit on the smaller side.  */
		int n_smaller_max = std::max (1, smaller / smaller_min);
		/* The number of boards that need to fit on the smaller side to make sure that the
		   above number is enough to fit all on screen.  */
		int n_small_1 = std::max (1, (n_elts + n_larger_max - 1) / n_larger_max);
		/* How many boards to actually fit on the smaller side: the minimum of the two
		   previously calculated numbers (so ideally the number we want, but not exceeding
		   the maximum).  */
		int n_small = std::min (n_small_1, n_smaller_max);
		int n_large = (n_elts + n_small - 1) / n_small;
		/* Compute picture sizes for filling the area.  */
		int ps1 = smaller / n_small;
		int ps2 = larger / n_large;
		// printf ("ps1 %d [%d %d] ps2 %d [%d %d] %% %d\n", ps1, smaller, n_small, ps2, larger, n_large, sb_size);
		if (w < h)
			ps2 -= extra_height;
		else
			ps1 -= extra_height;
		int min_ps = ps1 < min_size ? ps1 : std::min (ps1, ps2);
		if (min_ps >= min_size || (ps1 < min_size && (ps2 > ps1 || assume_sb))) {
			/* Everything fits (or can't fit, if the window is too narrow).
			   But try to find an even better layout.  */
			while (n_small < n_elts) {
				int n_small_new = n_small + 1;
				int n_large_new = (n_elts + n_small_new - 1) / n_small_new;
				int ps1b = smaller / n_small_new;
				int ps2b = larger / n_large_new;
				if (w < h)
					ps2b -= extra_height;
				else
					ps1b -= extra_height;
				int t = std::min (ps1b, ps2b);
				if (t < min_ps)
					break;
				min_ps = t;
				n_small++;
			}
			new_width = min_ps;
			new_height = min_ps + extra_height;
			break;
		} else {
			/* Use scrollbars and use the maximum width of the smaller side.  */
			if (assume_sb == 0) {
				smaller -= sb_size;
				continue;
			}
			// ps1 = std::max (ps1, min_size);
			new_width = ps1;
			new_height = ps1 + extra_height;
			break;
		}
	}

	int row = 0;
	int col = 0;
	int max_row = 0;
	int max_col = 0;
	for (auto p: previews) {
		p->x = col;
		p->y = row;
		max_row = std::max (max_row, row);
		max_col = std::max (max_col, col);
		if (w < h) {
			col += new_width;
			if (col + new_width > w) {
				col = 0;
				row += new_height;
			}
		} else {
			row += new_height;
			if (row + new_height > h) {
				row = 0;
				col += new_width;
			}
		}
	}
	view->setSceneRect (0, 0, max_col + new_width, max_row + new_height);

	return std::pair<int, int> { new_width, new_height };
}

void MainWindow_IGS::choices_resized ()
{
	size_t n_elts = m_previews.size ();

	if (n_elts == 0)
		return;

	QFontMetrics m (setting->fontStandard);
	int font_height = m.lineSpacing () + 1;

	std::vector<board_preview *> preview_ptrs (n_elts);
	for (size_t i = 0; i < n_elts; i++)
		preview_ptrs[i] = &m_previews[i];
	std::tie (m_preview_w, m_preview_h) = layout_previews (gameChoiceView, preview_ptrs, 2 * font_height);

	for (auto &p: m_previews) {
		update_preview (p.game, p.state);
	}
//	gameChoiceView->setSceneRect (m_preview_scene->itemsBoundingRect ());
}

void MainWindow_IGS::unobserve_game (go_game_ptr gr)
{
	using std::begin;
	using std::end;
	auto it = begin (m_previews);
	auto it_end = end (m_previews);
	bool choose_new = gr == m_game;
	while (it != it_end) {
		if (it->game == gr) {
			if (it->connector != nullptr)
				it->connector->slot_closeevent ();
			delete it->pixmap;
			m_previews.erase (it);
			break;
		}
		++it;
	}
	if (choose_new && !m_previews.empty ()) {
		board_clicked (m_previews[0].game, m_previews[0].connector, m_previews[0].state);
	}
	choices_resized ();
	if (m_previews.empty ()) {
		/* This is supposed to use deleteLater, and should therefore
		   be safe.  We used invokeMethod for a while, but not
		   everyone has a recent enough Qt yet.  */
		close ();
	}
}

void MainWindow_IGS::board_clicked (go_game_ptr gr, qGoBoard *connector, game_state *st)
{
	init_game_record (gr);
	gfx_board->set_displayed (st);
	disconnect (this, &MainWindow::signal_sendcomment, nullptr, nullptr);
	if (connector != nullptr) {
		connect (this, &MainWindow::signal_sendcomment, connector, &qGoBoard::slot_sendcomment);
		set_comment ("");
		for (const auto &c: connector->comments ())
			append_remote_comment (m_game, c.text, c.colored);
	}
	/* Ensure the cursor moves.  */
	for (const auto &p: m_previews)
		if (p.game == gr) {
			set_preview_cursor (p);
			break;
		}
}

void MainWindow_IGS::board_menu (QGraphicsSceneContextMenuEvent *e,
				 go_game_ptr gr, qGoBoard *, game_state *)
{
	QMenu menu;
	menu.addAction (tr ("Stop observing this game"), [this, gr] () { unobserve_game (gr); });
	menu.exec (e->screenPos ());
}

void MainWindow_IGS::update_settings ()
{
	MainWindow::update_settings ();
	if (game_mode () == modeObserveMulti)
		choices_resized ();
}

void MainWindow_IGS::remove_connector ()
{
	delete m_connector;
	m_connector = nullptr;
}

void MainWindow_IGS::resume_game (qGoBoard *connector, go_game_ptr old_game, go_game_ptr new_game, game_state *st)
{
	if (game_mode () == modeObserveMulti) {
		for (auto &p: m_previews)
			if (p.game == old_game) {
				p.game = new_game;
				p.connector = connector;
				p.active = true;
				p.state = st;
				choices_resized ();
				if (m_game == old_game) {
					init_game_record (new_game);
					set_game_position (st);
				}
				return;
			}
	} else {
		init_game_record (m_game);
		set_game_position (st);
	}
}

/*
 *	Playing or Observing
 */

qGoIF::qGoIF(QWidget *p) : QObject()
{
	qgo = new qGo();

	parent = p;
	gsName = GS_UNKNOWN;
}

qGoIF::~qGoIF()
{
	delete qgo;
}

void qGoIF::update_settings ()
{
	for (auto qb: boardlist)
		qb->update_settings ();
}

qGoBoard *qGoIF::find_game_id (int id)
{
	for (auto qb: boardlist)
		if (qb->get_id() == id && !qb->get_adj ()) {
			return qb;
		}
	return nullptr;
}

qGoBoard *qGoIF::find_game_players (const QString &pl1, const QString &pl2)
{
	for (auto qb: boardlist) {
		if (!qb->get_havegd () || qb->get_adj ())
			continue;
		const QString qb1 = qb->get_wplayer ();
		const QString qb2 = qb->get_bplayer ();
		if (qb1 == pl1 && (pl2.isEmpty () || qb2 == pl2))
			return qb;
		if (qb2 == pl1 && (pl2.isEmpty () || qb1 == pl2))
			return qb;
	}
	return nullptr;
}

void qGoIF::game_end (qGoBoard *qb, const QString &txt)
{
	if (qb == nullptr)
		return;

	// stopped game do not need a timer
	qb->set_stopTimer();

	if (txt == "-")
		return;

	qb->send_kibitz(txt + "\n");

	// set correct result entry
	QString rs;
	QString extended_rs = txt;

	if (txt.contains("White forfeits"))
		rs = "B+T";
	else if (txt.contains("Black forfeits"))
		rs = "W+T";
	else if (txt.contains("has run out of time"))
		rs = ((( txt.contains(myName) && qb->get_myColorIsBlack() ) ||
		       ( !txt.contains(myName) && !qb->get_myColorIsBlack() )) ?
		      "W+T" : "B+T");

	else if (txt.contains("W ",  Qt::CaseSensitive)
		 && txt.contains("B ",  Qt::CaseSensitive))
	{
		// NNGS: White resigns. W 59.5 B 66.0
		// IGS: W 62.5 B 93.0
		// calculate result first
		int posw, posb;
		float re1, re2;
		posw = txt.indexOf("W ");
		posb = txt.indexOf("B ");
		bool wfirst = posw < posb;

		if (!wfirst)
		{
			int h = posw;
			posw = posb;
			posb = h;
		}

		QString t1 = txt.mid(posw+1, posb-posw-2);
		QString t2 = txt.right(txt.length()-posb-1);
		re1 = t1.toFloat();
		re2 = t2.toFloat();

		re1 = re2-re1;
		if (re1 < 0)
		{
			re1 = -re1;
			wfirst = !wfirst;
		}

		if (re1 < 0.2)
		{
			rs = "Jigo";
			extended_rs = "        Jigo         ";
		}
		else if (wfirst)
		{
			rs = "B+" + QString::number(re1);
			extended_rs = "Black won by " + QString::number(re1) + " points";
		}
		else
		{
			rs = "W+" + QString::number(re1);
			extended_rs = "White won by " + QString::number(re1) + " points";
		}
	}
	else if (txt.contains("White resigns"))
		rs = "B+R";
	else if (txt.contains("Black resigns"))
		rs = "W+R";
	else if (txt.contains("has resigned the game"))
		rs = ((( txt.contains(myName) && qb->get_myColorIsBlack() ) ||
		       ( !txt.contains(myName) && !qb->get_myColorIsBlack() )) ?
		      "W+R" : "B+R");

	if (!rs.isNull())
	{
		qb->game_result (rs, extended_rs);
	}

	if (txt.contains("adjourned"))
	{
		if (qb->get_havegd ()) {
			qDebug() << "game adjourned... #" << QString::number(qb->get_id());
			qb->set_adj (true);
		}
	}

	n_observed--;
	client_window->update_observed_games (n_observed);
}

void qGoIF::game_end (const QString &id, const QString &txt)
{
	game_end (find_game_id (id.toInt ()), txt);
}

void qGoIF::game_end (const QString &pl1, const QString &pl2, const QString &txt)
{
	game_end (find_game_players (pl1, pl2), txt);
}

void qGoIF::handle_talk (const QString &pl, const QString &txt)
{
	for (auto qb: boardlist_disconnected)
		qb->try_talk (pl, txt);
}

// regular move info (parser)
void qGoIF::slot_move (GameInfo *gi)
{
	int game_id = gi->nr.toInt ();

	qGoBoard *b = find_game_id (game_id);
	if (b == nullptr) {
		static int last_warn_id = -1;
		if (last_warn_id != game_id)
			qWarning () << "no board found for move in game " << game_id;
		last_warn_id = game_id;
		return;
	}

	if (gi->mv_col == "T") {
		// set times
		b->setTimerInfo (gi->btime, gi->bstones, gi->wtime, gi->wstones);
	} else if (b->get_havegd ()) {
		stone_color sc = gi->mv_col == "B" ? black : white;

		b->set_move (sc, gi->mv_pt, gi->mv_nr);
	}
}

// start/adjourn/end/resume of a game (parser)
void qGoIF::slot_gamemove(Game *g)
{
	qGoBoard *qb;

	if (g->nr == "@")
		qb = find_game_players (myName, QString ());
	else
		qb = find_game_id (g->nr.toInt());

	if (qb == nullptr || qb->get_havegd ())
		return;

	stone_color own_color = none;
	GameMode mode = modeObserve;
	// check if it's my own game
	if (g->bname == myName || g->wname == myName) {
		own_color = g->bname == myName ? black : white;
		// case: trying to observe own game: this is the way to get back to match if
		// board has been closed by mistake
		if (g->bname == g->wname)
			mode = modeTeach;
		else
			mode = modeMatch;
	} else if (g->bname == g->wname && !g->bname.contains('*')) {
		// case: observing a teaching game
		qb->ExtendedTeachingGame = true;
		qb->IamTeacher = false;
		qb->IamPupil = false;
		qb->havePupil = false;
	}
	qb->set_game (g, mode, own_color);
}

// game to observe (mouse click)
void qGoIF::set_observe (const QString& gameno)
{
	int nr = gameno.toInt ();
	qGoBoard *b = find_game_id (nr);
	if (b != nullptr)
		return;

	b = new qGoBoard (this, nr);
	boardlist.append (b);

	client_window->sendcommand ("games " + gameno, false);
	client_window->sendcommand ("moves " + gameno, false);
	client_window->sendcommand ("all " + gameno, false);

	// set correct mode in qGo
	b->set_gsName (gsName);
	b->set_myName (myName);
	b->set_Mode_real (modeObserve);

	n_observed++;
	client_window->update_observed_games (n_observed);
}

/* Called before we create a window for a qGoBoard.  See if we have an adjourned
   game that matches this one so that we can resume rather than open a new window.  */
bool qGoBoard::matches_for_resume (const qGoBoard &other) const
{
	if (other.m_game->info ().name_b != m_game->info ().name_b
	    || other.m_game->info ().name_w != m_game->info ().name_w
	    || other.m_game->info ().komi != m_game->info ().komi)
		return false;

	game_state *our_st = m_game->get_root ();
	game_state *other_st = other.m_game->get_root ();
	/* Compare game records.  It's ok to have more moves in the
	   new version of the game.  They might have come in before we
	   started observing again.  */
	while (other_st != nullptr) {
		if (our_st == nullptr)
			return false;
		if (!our_st->get_board ().position_equal_p (other_st->get_board ()))
			return false;
		our_st = our_st->next_primary_move ();
		other_st = other_st->next_primary_move ();
	}
	/* At this point we know that we will use this board.  Transfer comments
	   to the new copy of the game record.  */
	our_st = m_game->get_root ();
	other_st = other.m_game->get_root ();
	while (other_st != nullptr) {
		our_st->set_comment (other_st->comment ());
		our_st = our_st->next_primary_move ();
		other_st = other_st->next_primary_move ();
	}
	return true;
}

qGoBoard *qGoIF::find_adjourned_game (const qGoBoard &new_game) const
{
	for (auto qb: boardlist) {
		if (qb->get_adj () && new_game.matches_for_resume (*qb))
			return qb;
	}
	return nullptr;
}

/* Called if the server informs us that a game has restarted (as indicated by a move number
   higher than the starting move).  Can only occur in non-quiet mode.
   We check the list of adjourned games to see if there is a game between these two players
   that we were observing before it adjourned.  */

void qGoIF::resume_observe (const QString &gameno, const QString &wname, const QString &bname)
{
	int nr = gameno.toInt ();

	/* First, see if we are already observing the new game.  That can happen if we are
	   trailing one of the players.  */
	for (auto qb: boardlist)
		if (!qb->get_adj () && qb->get_id () == nr)
			return;

	/* Now look if we were observing the old game.  */
	for (auto qb: boardlist) {
		if (qb->get_adj () && qb->get_bplayer() == bname && qb->get_wplayer() == wname)
		{
			qDebug("ok, adjourned game found ...");
			client_window->sendcommand ("observe " + gameno, false);
			return;
		}
	}
}

// a match is created
void qGoIF::create_match (const QString &gameno, const QString &opponent, bool resumed)
{
	GameMode mode = modeMatch;

	int nr = gameno.toInt ();
	/* At least with NNGS we are called multiple times for a single game start,
	   for both of the lines.
	   // 9 Match [5] with guest17 in 1 accepted.
	   // 9 Creating match [5] with guest17.
	   Don't create the board more than once.  */
	qGoBoard *b = find_game_id (nr);
	if (b != nullptr)
		return;

	b = new qGoBoard (this, nr);
	boardlist.append (b);

	if (opponent == myName) {
		mode = modeTeach;
		b->ExtendedTeachingGame = true;
		b->IamTeacher = true;
		b->havePupil = false;
		b->mark_set = false;
	}

	if (resumed) {
		client_window->sendcommand ("games " + gameno, false);
		client_window->sendcommand ("moves " + gameno, false);
		client_window->sendcommand ("all " + gameno, false);
	}
	else
		client_window->sendcommand ("games " + gameno, false);

	// set correct mode in qGo
	b->set_gsName (gsName);
	b->set_myName (myName);
	b->set_Mode_real (mode);

	n_observed++;
	client_window->update_observed_games (n_observed);
}

// a match is created
void qGoIF::resume_own_game (const QString &nr, const QString &wname, const QString &bname)
{
	create_match (nr, wname == myName ? bname : wname, true);
}

// remove all boards
void qGoIF::set_initIF ()
{
	for (auto b: boardlist) {
		b->disconnected (false);
	}
	boardlist.clear ();

	n_observed = 0;
	client_window->update_observed_games (n_observed);
}

void qGoIF::slot_matchsettings(const QString &id, const QString &handicap, const QString &komi, assessType kt)
{
	// seek board
	for (auto b: boardlist)
		if (b->get_id() == id.toInt()) {
			b->set_requests(handicap, komi, kt);
			qDebug() << QString("qGoIF::slot_matchsettings: h=%1, k=%2, kt=%3").arg(handicap).arg(komi).arg(kt);
			return;
		}

	qWarning("BOARD CORRESPONDING TO GAME SETTINGS NOT FOUND !!!");
}

void qGoIF::set_game_title (int nr, const QString &title)
{
	qGoBoard *qb = find_game_id (nr);
	if (qb)
		qb->set_title (title);
}

// komi set or requested
void qGoIF::slot_komi(const QString &nr, const QString &komi, bool isrequest)
{
	qGoBoard *qb;
	static int move_number_memo = -1;
	static QString komi_memo;

	// correctness:
	if (komi.isEmpty())
		return;

	if (isrequest)
	{
		// check if opponent is me
		if (nr.isEmpty() || nr == myName)
			return;

		// 'nr' is opponent (IGS/NNGS)
		qb = find_game_players (nr, QString ());

		if (qb)
		{
			// check if same opponent twice (IGS problem...)
			if (move_number_memo == qb->get_mv_counter () && !komi_memo.isEmpty () && komi_memo == komi)
			{
				qDebug() << QString("...request skipped: opponent %1 wants komi %2").arg(nr).arg(komi);
				return;
			}

			// remember of actual values
			move_number_memo = qb->get_mv_counter();
			komi_memo = komi;

			if (qb->get_reqKomi() == komi && setting->readBoolEntry("DEFAULT_AUTONEGO"))
			{
				if (qb->get_currentKomi() != komi)
				{
					qDebug("GoIF::slot_komi() : emit komi");
					client_window->sendcommand ("komi " + komi, true);
				}
			}
			else
			{
				slot_requestDialog(tr("komi ") + komi, tr("decline"), QString::number(qb->get_id()), nr);
			}
		}

		return;
	}
	// own game if nr == NULL
	else if (nr.isEmpty())
	{
		if (myName.isEmpty())
		{
			// own name not set -> should never happen!
			qWarning("*** Wrong komi because don't know which online name I have ***");
			return;
		}

		for (auto b: boardlist)
			if (b->get_havegd () && (b->get_wplayer () == myName || b->get_bplayer () == myName)) {
				b->set_komi (komi);
				break;
			}
	}
	else
	{
		// title message follows to move message
		for (auto b: boardlist)
			if (b->get_id () == nr.toInt ()) {
				b->set_komi (komi);
				break;
			}
	}

}

// game free/unfree message received
void qGoIF::slot_freegame(bool freegame)
{
	// what if 2 games (IGS) and mv_counter < 3 in each game? -> problem
	for (auto qb: boardlist)
		if (qb->get_havegd ()
		    && (qb->get_wplayer() == myName || qb->get_bplayer() == myName)
		    && qb->get_mv_counter() < 3)
		{
			qb->set_freegame(freegame);
			return;
		}
}

void qGoIF::remove_board (qGoBoard *qb)
{
	boardlist.removeOne (qb);
	boardlist_disconnected.append (qb);
}

void qGoIF::remove_disconnected_board (qGoBoard *qb)
{
	boardlist_disconnected.removeOne (qb);
}

// board window closed...
void qGoIF::window_closing (qGoBoard *qb)
{
	int id = qb->get_id();

	qDebug() << "qGoIF::slot_closeevent() -> game " << id << "\n";

	// destroy timers
	qb->set_stopTimer();

	if (id < 0 && id > -10000)
	{
		switch (qb->get_Mode())
		{
			case modeObserve:
				// unobserve
				if (gsName == IGS)
					client_window->sendcommand ("unobserve " + QString::number(-id), false);
				else
					client_window->sendcommand ("observe " + QString::number(-id), false);

				n_observed--;
				client_window->update_observed_games (n_observed);
				break;

			case modeMatch:
				n_observed--;
				client_window->update_observed_games (n_observed);
				break;

			case modeTeach:
				client_window->sendcommand ("adjourn", false);

				n_observed--;
				client_window->update_observed_games (n_observed);
				break;

			default:
				break;
		}
	}

	delete qb;
}

// kibitz strings
void qGoIF::slot_kibitz(int num, const QString& who, const QString& msg)
{
	qGoBoard *qb = nullptr;
	QString name;

	// own game if num == NULL
	if (num) {
		qb = find_game_id (num);
		name = who;
	} else {
		if (myName.isEmpty())
		{
			// own name not set -> should never happen!
			qWarning("*** qGoIF::slot_kibitz(): Don't know my online name ***");
			return;
		}
		qb = find_game_players (myName, QString ());
		if (qb != nullptr) {
			if (qb->get_wplayer() == myName)
				name = qb->get_bplayer ();
			else if (qb->get_bplayer() == myName)
				name = qb->get_wplayer ();

			// sound for "say" command
			qgo->playSaySound();
		}
	}

	if (!qb)
		qDebug("Board to send kibitz string not in list...");
	else if (!num && !who.isEmpty())
	{
		// special case: opponent has resigned - interesting in quiet mode
		qb->send_kibitz(msg);
		qDebug () << "strange opponent resignation\n";
	}
	else
		qb->send_kibitz(name + ": " + msg + "\n");
}

void qGoIF::slot_requestDialog(const QString &yes, const QString &no, const QString & /*id*/, const QString &opponent)
{
	QString opp = opponent;
	if (opp.isEmpty())
		opp = tr("Opponent");

	if (no.isEmpty())
	{
		QMessageBox mb(tr("Request of Opponent"),
			tr("%1 wants to %2\nYES = %3\nCANCEL = %4").arg(opp).arg(yes).arg(yes).arg(tr("ignore request")),
			QMessageBox::NoIcon,
			QMessageBox::Yes | QMessageBox::Default,
			QMessageBox::Cancel | QMessageBox::Escape,
			Qt::NoButton);
		mb.activateWindow();
		mb.raise();
		qgo->playPassSound();

		if (mb.exec() == QMessageBox::Yes)
		{
			qDebug() << QString("qGoIF::slot_requestDialog(): emit %1").arg(yes);
			client_window->sendcommand (yes, false);
		}
	}
	else
	{
		QMessageBox mb(tr("Request of Opponent"),
			//tr("%1 wants to %2\nYES = %3\nCANCEL = %4").arg(opp).arg(yes).arg(yes).arg(no),
			tr("%1 wants to %2\n\nDo you accept ? \n").arg(opp).arg(yes),
			QMessageBox::Question,
			QMessageBox::Yes | QMessageBox::Default,
			QMessageBox::No | QMessageBox::Escape,
			Qt::NoButton);
		mb.activateWindow();
		mb.raise();
		qgo->playPassSound();

		if (mb.exec() == QMessageBox::Yes)
		{
			qDebug() << QString("qGoIF::slot_requestDialog(): emit %1").arg(yes);
			client_window->sendcommand (yes, false);
		}
		else
		{
			qDebug() << QString("qGoIF::slot_requestDialog(): emit %1").arg(no);
			client_window->sendcommand (no, false);
		}
	}
}

void qGoIF::slot_removestones(const QString &pt, const QString &game_id)
{
	qGoBoard *qb = nullptr;

	if (pt.isEmpty() && game_id.isEmpty())
	{
		qDebug("slot_removestones(): !pt !game_id");
		// search game
		for (auto b: boardlist)
			if (b->get_Mode() == modeMatch) {
				qb = b;
				break;
			}

		if (!qb)
		{
			qWarning("*** No Match found !!! ***");
			return;
		}

		switch (gsName)
		{
			case IGS:
				/* Can be a re-enter if already scoring.  */
				qb->enter_scoring_mode (true);
				break;

			default:
				if (qb->get_Mode() == modeMatch)
					qb->enter_scoring_mode (false);
				else
					qb->leave_scoring_mode ();
				break;
		}

		return;
	}

	if (!pt.isEmpty() && game_id.isEmpty())
	{
		qDebug("slot_removestones(): pt !game_id");
		// stone coords but no game number:
		// single match mode, e.g. NNGS
		for (auto b: boardlist)
			if (b->get_Mode () == modeMatch || b->get_Mode () == modeTeach) {
				qb = b;
				break;
			}
	}
	else
	{
qDebug("slot_removestones(): game_id");
		// multi match mode, e.g. IGS
		for (auto b: boardlist)
			if (b->get_id () == game_id.toInt ()) {
				qb = b;
				qb->enter_scoring_mode (false);
				break;
			}
	}

	if (!qb)
	{
		qWarning("*** No Match found !!! ***");
		return;
	}

	int i = pt[0].toLatin1() - 'A' + 1;
	// skip j
	if (i > 8)
		i--;

	int j;
	if (pt.length () > 2 && pt[2].toLatin1() >= '0' && pt[2].toLatin1() <= '9')
		j = qb->get_boardsize() + 1 - pt.mid(1,2).toInt();
	else
		j = qb->get_boardsize() + 1 - pt[1].digitValue();

	// mark dead stone
	qb->mark_dead_stone (i - 1, j - 1);
	qb->send_kibitz("removing @ " + pt + "\n");
}

// game status received
void qGoIF::slot_result(const QString &txt, const QString &line, bool isplayer, const QString &komi)
{
	static qGoBoard *qb;
	static int column;

	if (isplayer)
	{
		column = 0;
		qb = find_game_players (txt, QString ());
		if (qb)
			qb->receive_score_begin ();
	}
	else if (qb)
	{
		qb->receive_score_line (column, line);
		column++;
		if (txt.toInt() == (int) line.length() - 1)
			qb->receive_score_end ();
	}
}

// undo a move
void qGoIF::slot_undo(const QString &player, const QString &move)
{
	qDebug() << "qGoIF::slot_undo()" << player << move;

	// check if game number given
	bool ok;
	int nr = player.toInt(&ok);
	qGoBoard *qb = ok ? find_game_id (nr) : find_game_players (player, QString ());
	if (qb != nullptr) {
		qb->remote_undo (move);
		return;
	}
	qWarning("*** board for undo not found!");
}

void qGoBoard::remote_undo (const QString &)
{
	if (win == nullptr || m_state->root_node_p ()) {
		/* @@@ I've observed an undo without a window, and before moves were
		   sent. Sadly, there was a debugger running, but no log file, so
		   it's unclear what the server was trying to communicate.
		   Log it and hope we see it again.  */
		qWarning () << "unexpected undo";
		return;
	}

	if (!m_scoring)
	{
		dec_mv_counter();
		game_state *prev = m_state->prev_move ();
		win->transfer_displayed (m_game, m_state, prev);
		m_state->disconnect ();
		m_state = prev;
		win->update_game_tree (m_game);
		return;
	}

	// back to matchMode
	leave_scoring_mode ();
	win->setGameMode (modeMatch);
	win->nav_previous_move ();
	dec_mv_counter ();
	send_kibitz (tr ("GAME MODE: place stones..."));
}

void qGoBoard::enter_scoring_mode (bool may_reenter)
{
	if (m_scoring) {
		if (may_reenter) {
			leave_scoring_mode ();
		}
	}
	if (m_scoring)
		return;
	send_kibitz (tr ("SCORE MODE: click on a stone to mark as dead...") + "\n");
	m_scoring = true;
	win->setGameMode (modeScoreRemote);
}

void qGoBoard::leave_scoring_mode ()
{
	/* ??? Maybe we don't even need to set the game mode here.  After all
	   we should enter modeNormal or modePostMatch pretty soon after this.  */
	if (gameMode == modeMatch || gameMode == modeTeach)
		win->setGameMode (gameMode);
	m_scoring = false;
}

void qGoBoard::mark_dead_stone (int x, int y)
{
	win->mark_dead_external (x, y);
}

// time has been added
void qGoIF::slot_timeAdded(int time, bool toMe)
{
	for (auto qb: boardlist) {
		/* @@@ Not a great test.  */
		if (qb->get_Mode () == modeMatch) {

			// use time == -1 to pause the game
			if (time == -1)
				qb->set_gamePaused(toMe);
			else if ((toMe && qb->get_myColorIsBlack()) || (!toMe && !qb->get_myColorIsBlack()))
				qb->addtime_b(time);
			else
				qb->addtime_w(time);
			return;
		}
	}

	qWarning("*** board for undo not found!");
}

void qGoIF::observer_list_start (int id)
{
	qGoBoard *b = find_game_id (id);
	if (b)
		b->observer_list_start ();
}

void qGoIF::observer_list_entry (int id, const QString &n, const QString &r)
{
	qGoBoard *b = find_game_id (id);
	if (b)
		b->observer_list_entry (n, r);
}

void qGoIF::observer_list_end (int id)
{
	qGoBoard *b = find_game_id (id);
	if (b)
		b->observer_list_end ();
}

/*
 *	qGo Board
 */



// add new board window
qGoBoard::qGoBoard(qGoIF *qif, int gameid) : m_qgoif (qif), id (gameid)
{
	qDebug("::qGoBoard()");
	adjourned = false;
	mv_counter = -1;
	requests_set = false;
	m_game = nullptr;
	win = nullptr;

	ExtendedTeachingGame = false;
	IamTeacher = false;
	IamPupil = false;
	havePupil = false;
	haveControls = false;
	mark_set = false;

	// set timer to 1 second
	timer_id = startTimer(1000);
	game_paused = false;
	req_handicap = QString ();
	req_komi = -1;
	bt_i = -1;
	wt_i = -1;
	stated_mv_count = 0;
	update_settings ();

	m_observers.setColumnCount (2);
	m_observers.setHorizontalHeaderItem (0, new QStandardItem ("Name"));
	m_observers.setHorizontalHeaderItem (1, new QStandardItem ("Rk"));
	m_observers.setSortRole (Qt::UserRole + 1);
}

qGoBoard::~qGoBoard()
{
	qDebug("~qGoBoard()");
	if (m_connected)
		m_qgoif->remove_board (this);
	m_qgoif->remove_disconnected_board (this);

	delete m_title;
	delete m_scoring_board;
}

void qGoBoard::update_settings ()
{
	m_warn_time = setting->readIntEntry ("BY_TIMER");
	m_divide_timer = setting->readBoolEntry ("TIME_WARN_DIVIDE");
}

void qGoBoard::observer_list_start ()
{
	if (win) {
		win->cb_opponent->clear();
		win->TextLabel_opponent->setText(tr("opponent:"));
		win->cb_opponent->addItem(tr("-- none --"));
		if (havePupil && ttOpponent != tr("-- none --"))
		{
			win->cb_opponent->addItem(ttOpponent, 1);
			win->cb_opponent->setCurrentIndex(1);
		}
	}

	m_observers.setRowCount (0);
}

void qGoBoard::observer_list_entry (const QString &n, const QString &r)
{
	QList<QStandardItem *> list;
	QStandardItem *nm_item = new QStandardItem (n);
	nm_item->setData (n, Qt::UserRole + 1);
	list.append (nm_item);
	QStandardItem *rk_item = new QStandardItem (r);
	rk_item->setData (rkToKey (r), Qt::UserRole + 1);
	list.append (rk_item);
	m_observers.appendRow (list);
	if (win) {
		win->cb_opponent->addItem (n);
		int cnt = win->cb_opponent->count();
		win->TextLabel_opponent->setText(tr("opponent:") + " (" + QString::number(cnt-1) + ")");
	}
}

void qGoBoard::observer_list_end ()
{
}

void qGoBoard::receive_score_begin ()
{
	m_scoring_board = new go_board (m_state->get_board ());
}

void qGoBoard::receive_score_end ()
{
	leave_scoring_mode ();

	game_state *st = m_state;
	m_scoring_board->territory_from_markers ();
	game_state *st_new = st->add_child_edit (*m_scoring_board, m_state->to_move (), true);
	if (win != nullptr) {
		win->transfer_displayed (m_game, st, st_new);
		win->update_game_tree (m_game);
	}
	m_state = st_new;
	delete m_scoring_board;
	m_scoring_board = nullptr;
}

void qGoBoard::receive_score_line (int n, const QString &line)
{
	// ok, now mark territory points only
	// 0 .. black
	// 1 .. white
	// 2 .. free
	// 3 .. neutral
	// 4 .. white territory
	// 5 .. black territory

	for (int y = 0; y < line.length (); y++)
		switch (line[y].digitValue ()) {
		case 4:
			m_scoring_board->set_mark (n, y, mark::terr, 0);
			break;
		case 5:
			m_scoring_board->set_mark (n, y, mark::terr, 1);
			break;
		default:
			break;
		}
}

/* Called when we have game info and all the moves up to this point.  */
void qGoBoard::game_startup ()
{
	bool resumed = false;
	/* See if we can reuse an existing window when resuming observation of an adjourned game.  */
	if (gameMode == modeObserve) {
		qGoBoard *other = m_qgoif->find_adjourned_game (*this);
		if (other) {
			go_game_ptr other_game = other->m_game;
			win = other->win;
			other->win = nullptr;
			other->disconnected (true);
			win->resume_game (this, other_game, m_game, m_state);
			resumed = true;
		}
	}

	if (win == nullptr) {
		bool am_black = m_own_color == black;
		bool am_white = m_own_color == white;
		GameMode win_mode = gameMode == modeObserve && !setting->readBoolEntry ("OBSERVE_SINGLE") ? modeObserveMulti : gameMode;
		if (win_mode == modeObserveMulti && multi_observer_win != nullptr) {
			win = multi_observer_win;
			multi_observer_win->add_game (m_game, this, m_state);
		} else
			win = new MainWindow_IGS (0, m_game, screen_key (client_window), this, am_white, am_black, win_mode);
		game_state *root = m_game->get_root ();
		if (root != m_state)
			win->transfer_displayed (m_game, root, m_state);
		win->show ();
	}
	win->set_observer_model (&m_observers);

	if (m_comments.size () > 0)
		for (auto &c: m_comments)
			win->append_remote_comment (m_game, c.text, c.colored);

	if (resumed)
		send_kibitz (tr ("Game continued as game number %1\n").arg (id));
}

// set game info to one board
void qGoBoard::set_game(Game *g, GameMode mode, stone_color own_color)
{
	go_board startpos (g->Sz.toInt ());
	int handi = g->H.toInt ();
	if (handi >= 2)
		startpos = new_handicap_board (g->Sz.toInt(), handi);
	enum ranked rt = ranked::ranked;
	if (g->FR.contains("F"))
		rt = ranked::free;
	else if (g->FR.contains("T"))
		rt = ranked::teaching;

	int timelimit = 0;
	std::string overtime = "";
	if (rt != ranked::teaching)
	{
		int byoTime = g->By.toInt() * 60;
		if (wt_i > 0)
			timelimit = (((wt_i > bt_i ? wt_i : bt_i) / 60) + 1) * 60;
		else
			timelimit = 60;

		if (byoTime != 0)
			overtime = "25/" + std::to_string (byoTime) + " Canadian";
	}
	/* This needs to be saved, because it will be used in modePostMatch when the
	   user may already edit the game information.  */
	m_opp_name = own_color == white ? g->bname : g->wname;

	std::string place = "";
	switch (gsName)
	{
	case IGS:
		place = "IGS";
		break;
	case LGS:
		place = "LGS";
		break;
	case WING:
		place = "WING";
		break;
	default:
		break;
	}

	game_info info;
	info.title = m_title ? m_title->toStdString () : "";
	info.name_w = g->wname.toStdString ();
	info.name_b = g->bname.toStdString ();
	info.rank_w = g->wrank.toStdString ();
	info.rank_b = g->brank.toStdString ();
	info.komi = g->K.toFloat ();
	info.handicap = handi;
	info.rated = rt;
	info.date = QDate::currentDate().toString("yyyy-MM-dd").toStdString ();
	info.place = place;
	info.time = std::to_string (timelimit);
	info.overtime = overtime;

	m_game = std::make_shared<game_record> (startpos, handi >= 2 ? white : black, info);
	m_state = m_game->get_root ();
	if (g->FR.contains("F"))
		m_freegame = FREE;
	else if (g->FR.contains("T"))
		m_freegame = TEACHING;
	else
		m_freegame = RATED;

	// needed for correct sound
	stated_mv_count = g->mv.toInt();
	set_Mode_real (mode);
	m_own_color = own_color;

	if (stated_mv_count == 0)
		game_startup ();
}

void qGoBoard::set_title(const QString &t)
{
	if (m_title)
		delete m_title;
	m_title = new QString (t);

	if (m_game == nullptr)
		return;

	m_game->set_title (t.toStdString ());
	if (win)
		win->update_game_record ();
}

void qGoBoard::set_komi(const QString &k)
{
	QString k_;
	// check whether the string contains two commas
	if (k.count('.') > 1)
		k_ = k.left(k.length()-1);
	else
		k_ = k;

	qDebug() << "set komi to " << k_;

#if 0
	gd.komi = k_.toFloat();
	win->getBoard()->setGameData(&gd);
	win->normalTools->komi->setText(k_);
#endif
}

void qGoBoard::set_freegame(bool f)
{
	m_game->set_ranked_type (f ? ranked::free : ranked::ranked);
	win->update_game_record ();
}

// send regular time Info
void qGoBoard::timerEvent(QTimerEvent*)
{
	// wait until first move
	if (win == nullptr || win->active_board () != this || mv_counter < 0 || id < 0 || game_paused)
		return;

	if (m_state->to_move () == black)
	{
		// B's turn
		bt_i--;
		int st = b_stones.toInt ();
		st = st < 1 ? 1 : st;
		int warn_time = m_warn_time * (m_divide_timer ? st : 1);
		bool warn = bt_i > - 1 && bt_i <= warn_time;
		win->setTimes (bt_i, b_stones, wt_i, w_stones, warn, false, bt_i);
	}
	else
	{
		// W's turn
		wt_i--;
		int st = w_stones.toInt ();
		st = st < 1 ? 1 : st;
		int warn_time = m_warn_time * (m_divide_timer ? st : 1);
		bool warn = wt_i > - 1 && wt_i <= warn_time;
		win->setTimes (bt_i, b_stones, wt_i, w_stones, false, warn, wt_i);
	}
}

// stop timer
void qGoBoard::set_stopTimer()
{
	if (timer_id)
		killTimer(timer_id);

	timer_id = 0;
}

// send regular time info
void qGoBoard::setTimerInfo (const QString &btime, const QString &bstones, const QString &wtime, const QString &wstones)
{
	bt_i = btime.toInt ();
	wt_i = wtime.toInt ();
	b_stones = bstones;
	w_stones = wstones;

	if (m_game != nullptr)
		update_time_info (m_state);
}

// addtime function
void qGoBoard::addtime_b(int m)
{
	bt_i += m*60;
	win->setTimes (bt_i, b_stones, wt_i, w_stones, false, false, 0);
}

void qGoBoard::addtime_w(int m)
{
	wt_i += m*60;
	win->setTimes (bt_i, b_stones, wt_i, w_stones, false, false, 0);
}

void qGoBoard::set_Mode_real(GameMode mode)
{
	gameMode = mode;
}

void qGoBoard::update_time_info (game_state *st)
{
	if (!st->was_move_p ())
		return;
	stone_color sc = st->get_move_color ();
	qDebug () << "updating time for " << (sc == black ? "black" : "white");
	st->set_time_left (sc, std::to_string (sc == white ? wt_i : bt_i));
	QString &whichstones = sc == white ? w_stones : b_stones;
	if (whichstones != "-1")
		st->set_stones_left (sc, whichstones.toStdString ());
}

void qGoBoard::set_move(stone_color sc, QString pt, QString mv_nr)
{
	int mv_nr_int;

	// IGS: case undo with 'mark': no following command
	// -> from qgoIF::slot_undo(): set_move(stoneNone, 0, 0)
	if (mv_nr.isEmpty())
		// undo one move
		mv_nr_int = mv_counter - 1;
	else
		mv_nr_int = mv_nr.toInt();

	if (mv_nr_int > mv_counter)
	{
		if (mv_nr_int != mv_counter + 1 && mv_counter != -1)
			// case: move has done but "moves" cmd not executed yet
			qWarning("**** LOST MOVE !!!! ****");
		else if (mv_counter == -1 && mv_nr_int != 0)
		{
			qDebug("move skipped");
			// skip moves until "moves" cmd executed
			return;
		}
		else
			mv_counter++;
	}
	else if (mv_nr_int + 1 == mv_counter)
	{
		// scoring mode? (NNGS)
		if (gameMode == modeScore)
		{
			leave_scoring_mode ();
		}

		// special case: undo handicap
		if (mv_counter <= 0 && m_game->info ().handicap > 0)
		{
			go_board new_root = new_handicap_board (m_game->boardsize (), 0);
			m_game->replace_root (new_root, black);
			m_game->set_handicap (0);
			qDebug("set Handicap to 0");
		}

		if (pt.isEmpty())
		{
			// case: undo
			qDebug() << "set_move(): UNDO in game " << QString::number(id);
			qDebug() << "...mv_nr = " << mv_nr;
			remote_undo ("");

			mv_counter--;
		}

		return;
	}
	else
		// move already done...
		return;

	if (pt.contains("Handicap"))
	{
		QString handi = pt.simplified();
		int h = handi.section(' ', 1, 1).toInt();

		// check if handicap is set with initGame() - game data from server do not
		// contain correct handicap in early stage, because handicap is first move!
		if (m_game->info ().handicap != h)
		{
			m_game->set_handicap (h);
			go_board new_root = new_handicap_board (m_game->boardsize (), h);
			m_game->replace_root (new_root, h > 1 ? white : black);
			qDebug ("corrected Handicap");
		}
	} else if (pt.contains ("Pass", Qt::CaseInsensitive)) {
		game_state *st = m_state;
		game_state *st_new = m_state->add_child_pass ();
		if (win != nullptr) {
			win->transfer_displayed (m_game, st, st_new);
			win->update_game_tree (m_game);
			win->playPassSound ();
		}
		m_state = st_new;
	} else {
		if (gameMode == modeMatch && mv_counter < 2 && m_own_color == white)
		{
			// if black has not already done - maybe too late here???
			if (requests_set)
			{
				qDebug() << "qGoBoard::set_move() : check_requests at move " << mv_counter;
				check_requests();
			}
		}

		int i = pt[0].toLatin1() - 'A' + 1;
		// skip j
		if (i > 8)
			i--;

		int j;

		if (pt.length () > 2 && pt[2].toLatin1() >= '0' && pt[2].toLatin1() <= '9')
			j = m_game->boardsize () + 1 - pt.mid(1,2).toInt();
		else
			j = m_game->boardsize () + 1 - pt[1].digitValue();

		game_state *st = m_state;
		/* The only way we can get a dup is if live analysis is running and we added a diagram with a
		   shift-click.  We don't want the node marked as a diagram to become part of the main line.
		   Hence, allow dups here when adding the move.  */
		game_state *st_new = st->add_child_move (i - 1, j - 1, sc, game_state::add_mode::set_active, true);
		if (st_new != nullptr) {
			st->make_child_primary (st_new);
			if (win != nullptr) {
				win->transfer_displayed (m_game, st, st_new);
				win->update_game_tree (m_game);
				win->playClick ();
			}
			m_state = st_new;
		} else {
			/* Invalid move.  @@@ do something sensible.  */
		}

		if (st_new != nullptr && stated_mv_count <= mv_counter && wt_i >= 0 && bt_i >= 0)
			update_time_info (st_new);
	}
	qDebug () << "found move " << mv_counter << " of " << stated_mv_count << "\n";
	if (mv_counter + 1 == stated_mv_count && win == nullptr)
		game_startup ();
}

// board window closed
void qGoBoard::slot_closeevent()
{
	/* Don't delete it later.  */
	win = 0;
	if (id > 0)
	{
		id = -id;
		m_qgoif->window_closing (this);
	}
	else
		qWarning("id < 0 ******************");
}

void qGoBoard::disconnected (bool remove_from_list)
{
	if (remove_from_list && m_connected)
		m_qgoif->remove_board (this);
	m_connected = false;
	set_stopTimer();

	qgo->playGameEndSound ();
	m_observers.clear ();

	// set board editable...
	GameMode new_mode = gameMode == modeMatch ? modePostMatch : modeNormal;
	set_Mode_real (new_mode);
	if (win)
		win->end_game (m_game, this, new_mode);
	/* @@@ Sometimes we get a game result without moves, if we started observing just
	   as the game ended.  We should arrange for some way to delete this qGoBoard.  */
}

void qGoBoard::add_postgame_break ()
{
	if (gameMode != modePostMatch || m_postgame_chat || !win)
		return;

	m_postgame_chat = true;
	QString to_add = "------------------------\n" + tr ("Post-game discussion:") + "\n";
	win->append_remote_comment (m_game, to_add, false);
}

void qGoBoard::try_talk (const QString &pl, const QString &txt)
{
	if (gameMode != modePostMatch || pl != m_opp_name)
		return;

	add_postgame_break ();
	QString to_add = pl + ": " + txt + "\n";

	if (m_state != nullptr) {
		const std::string old_comment = m_state->comment ();
		m_state->set_comment (old_comment + to_add.toStdString ());
	}

	append_text (to_add, false);
	if (win)
		win->append_remote_comment (m_game, to_add, false);
}

void qGoBoard::append_text (const QString &msg, bool colored)
{
	if (!m_comments.empty () && m_comments.back ().colored == colored) {
		m_comments.back ().text += msg;
		return;
	}
	m_comments.push_back (text_piece { msg, colored });
}

// write kibitz strings to comment window
void qGoBoard::send_kibitz (const QString &msg, bool own)
{
	// if observing a teaching game
	if (ExtendedTeachingGame && !IamTeacher)
	{
		// get msgs from teacher
		if (msg.indexOf("#OP:") != -1)
		{
			// opponent has been selected
			QString opp;
			opp = msg.section(':', 2, -1).remove(QRegExp("\\(.*$")).trimmed();

			if (opp == "0")
				slot_ttOpponentSelected(tr("-- none --"));
			else
				slot_ttOpponentSelected(opp);
			return;
		}
		else if (IamPupil)
		{
			if (msg.indexOf("#TC:ON") != -1)
			{
				// teacher gives controls to pupil
				haveControls = true;
				win->pb_controls->setEnabled(true);
				win->pb_controls->setChecked(true);
				return;
			}
			else if (msg.indexOf("#TC:OFF") != -1)
			{
				// teacher takes controls back
				haveControls = false;
				win->pb_controls->setDisabled(true);
				win->pb_controls->setChecked(false);
				return;
			}
		}
	}
	else if (ExtendedTeachingGame && IamTeacher)
	{
		// get msgs from pupil
		if (msg.indexOf("#OC:OFF") != -1)
		{
			// pupil gives controls back
			haveControls = true;
			win->pb_controls->setChecked(false);
			return;
		}
		else if (havePupil
			 && msg.section(' ', 0, 0) == ttOpponent.section(' ', 0, 0)
			 && ((myColorIsBlack && (mv_counter % 2))
			     || (!myColorIsBlack && ((mv_counter % 2 + 1)
						     || mv_counter == -1))
			     || !haveControls))
		{
			// move from opponent - it's his turn (his color or he has controls)!
			// xxx [rk]: B1 (7)
			//   it's ensured that yyy [rk] didn't send forged message
			//   e.g.: yyy [rk]: xxx[rk]: S1 (3)
			QString s;
			s = msg.section(':', 1, -1).remove(QRegExp("\\(.*$")).trimmed().toUpper();

			// check whether it's a position
			// e.g. B1, A17, NOT: ok, yes
			if (s.length() < 4 && s[0] >= 'A' && s[0] < 'U' && s[1] > '0' && s[1] <= '9')
			{
				// ok, could be a real pt
				// now teacher has to send it: pupil -> teacher -> server
				if (gsName == IGS)
					client_window->sendcommand (s + " " + QString::number(id), false);
				else
					client_window->sendcommand (s, false);
				return;
			}
		}
	}

	// skip my own messages
//	qDebug() << "msg.indexOf(myName) = " << QString::number(msg.indexOf(myName));
//	if (msg.indexOf(myName) == 0)
//		return;

	add_postgame_break ();

	// normal stuff...
	QString to_add = msg;
	if (!to_add.contains(QString::number(mv_counter + 1)))
		to_add = "(" + QString::number(mv_counter + 1) + ") " + to_add;
	if (m_state != nullptr) {
		const std::string old_comment = m_state->comment ();
		m_state->set_comment (old_comment + to_add.toStdString ());
	}
	append_text (to_add, own);
	if (win)
		win->append_remote_comment (m_game, to_add, own);
}

void qGoBoard::slot_sendcomment(const QString &comment)
{
	// # has to be at the first position
	if (comment.indexOf("#") == 0)
	{
		qDebug("detected (#) -> send command");
		QString testcmd = comment;
		testcmd = testcmd.remove(0, 1).trimmed();
		client_window->sendcommand (testcmd, true);
		return;
	}

	if (gameMode == modePostMatch) {
		client_window->sendcommand ("tell " + m_opp_name + " " + comment, false);
	} else if (gameMode == modeObserve || gameMode == modeTeach || gameMode == modeMatch && ExtendedTeachingGame) {
		// kibitz
		client_window->sendcommand ("kibitz " + QString::number(id) + " " + comment, false);
	} else {
		// say
		client_window->sendcommand ("say " + comment, false);
	}
	send_kibitz("-> " + comment + "\n", true);
}

void qGoBoard::send_coords (int x, int y)
{
	if (id < 0)
		return;

	if (x > 7)
		x++;
	QChar c1 = x + 'A';
	int c2 = m_game->boardsize () - y;

	if (ExtendedTeachingGame && IamPupil)
		client_window->sendcommand ("kibitz " + QString::number(id) + " " + QString(c1) + QString::number(c2), false);
	else
		client_window->sendcommand (QString(c1) + QString::number(c2) + " " + QString::number(id), false);
}

void qGoBoard::player_toggle_dead (int x, int y)
{
	send_coords (x, y);
}

void qGoBoard::move_played (game_state *st, int x, int y)
{
	m_state = st;
	update_time_info (m_state);
	send_coords (x, y);
}

void qGoBoard::game_result (const QString &rs, const QString &extended_rs)
{
	/* We can get here without having seen moves - observing a game just as it ends may cause the
	   server to send a result without moves.  Be extra careful.  */
	if (m_game.get () == nullptr) {
		qWarning () << "Result before game was set up";
		return;
	}

	m_game->set_result (rs.toStdString ());
	send_kibitz (rs + "\n");
	bool autosave = setting->readBoolEntry (gameMode == modeObserve ? "AUTOSAVE" : "AUTOSAVE_PLAYED");
	bool autosave_to_path = setting->readBoolEntry ("AUTOSAVE_TO_PATH");

	if (autosave && win)
	{
		QString path = autosave_to_path ? setting->readEntry ("AUTOSAVE_PATH") : "";
		/* Only use path if it is valid.  */
		if (!path.isEmpty () && !QDir (path).exists ())
			path = "";
		QString filename = get_candidate_filename (path, m_game->info ());
		save_to_file (win, m_game, filename);
	}

	GameMode prev_mode = gameMode;
	id = -id;

	disconnected (true);

	if (prev_mode != modeObserve)
		QMessageBox::information(win, tr("Game #") + QString::number(id), extended_rs);

	qDebug() << "Result: " << rs;
}

void qGoBoard::doPass()
{
	if (id > 0)
		client_window->sendcommand ("pass", false);
}

void qGoBoard::doResign()
{
	if (id > 0)
		client_window->sendcommand ("resign", false);
}

void qGoBoard::doUndo()
{
	if (id > 0) {
		if (gsName ==IGS)
			client_window->sendcommand ("undoplease", false);
		else
			client_window->sendcommand ("undo", false);
	}
}

void qGoBoard::doAdjourn()
{
	qDebug("button: adjourn");
	if (id > 0)
		client_window->sendcommand ("adjourn", false);
}

void qGoBoard::doDone()
{
	if (id > 0)
		client_window->sendcommand ("done", false);
}

void qGoBoard::set_requests(const QString &handicap, const QString &komi, assessType kt)
{
qDebug() << "set req_handicap to " << handicap;
qDebug() << "set req_komi to " << komi;
	req_handicap = handicap;
	req_komi = komi;
	req_free = kt;
	requests_set = true;
}

// compare settings with requested values
void qGoBoard::check_requests()
{
	// check if handicap requested via negotiation (if activated!)
	// do not set any value while reloading a game
	if (req_handicap.isNull() || gameMode == modeTeach || setting->readBoolEntry("DEFAULT_AUTONEGO"))
		return;

	if (m_game->info ().handicap != req_handicap.toInt ())
	{
		client_window->sendcommand ("handicap " + req_handicap, false);
		qDebug() << "Requested handicap: " << req_handicap;
	}
	else
		qDebug("Handicap settings ok...");

	if (m_game->info ().komi != req_komi.toFloat ())
	{
		qDebug("qGoBoard::check_requests() : emit komi");
		client_window->sendcommand ("komi " + req_komi, false);
	}

	// if size != 19 don't care, it's a free game
	if (req_free != noREQ && m_game->boardsize () == 19 && mv_counter < 1)
	{
		// check if requests are equal to settings or if teaching game
		if ((m_freegame == FREE && req_free == FREE) ||
		    (m_freegame != FREE && req_free == RATED) ||
		    req_free == TEACHING)
		    return;

		switch (gsName)
		{
			case NNGS:
				if (m_freegame == FREE)
					client_window->sendcommand ("unfree", false);
				else
					client_window->sendcommand ("free", false);
				break;

			// default: toggle
			default:
				client_window->sendcommand ("free", false);
				break;
		}
	} else
		qDebug("Rated game settings ok...");
}

// click on time field
void qGoBoard::slot_addtimePauseB()
{
	if (m_own_color == black)
	{
		switch (gsName)
		{
			case IGS:
				break;

			default:
				if (game_paused)
					client_window->sendcommand ("unpause", false);
				else
					client_window->sendcommand ("pause", false);
				break;
		}
	}
	else
		client_window->sendcommand ("addtime 1", false);
}

// click on time field
void qGoBoard::slot_addtimePauseW()
{
	if (m_own_color == white)
	{
		switch (gsName)
		{
			case IGS:
				break;

			default:
				if (game_paused)
					client_window->sendcommand ("unpause", false);
				else
					client_window->sendcommand ("pause", false);
				break;
		}
	}
	else
		client_window->sendcommand ("addtime 1", false);
}

// for several reasons: let me know which is my color
void qGoBoard::set_myColorIsBlack(bool b)
{
	myColorIsBlack = b;
}

// teachtools - teacher has selected an opponent
void qGoBoard::slot_ttOpponentSelected(const QString &opponent)
{
	if (opponent == tr("-- none --"))
	{
		havePupil = false;
	}
	else
	{
		ttOpponent = opponent;
		havePupil = true;
		// teacher has controls when opponent is selected
		haveControls = IamTeacher;
	}
	win->pb_controls->setEnabled(IamTeacher && havePupil);

	if (IamTeacher)
	{
		if (opponent == tr("-- none --"))
			client_window->sendcommand ("kibitz " + QString::number(id) + " #OP:0", false);
		else
			client_window->sendcommand ("kibitz " + QString::number(id) + " #OP:" + opponent, false);
		// set teacher's color -> but: teacher can play both colors anyway
		set_myColorIsBlack(mv_counter % 2 + 1);
	}
	else
	{
		bool found = false;
		// look for correct item
		QComboBox *cb = win->cb_opponent;
		int cnt = cb->count();
		for (int i = 0; i < cnt && !found; i++)
		{
			cb->setCurrentIndex(i);
			if (cb->currentText() == opponent)
			{
				cb->setCurrentIndex(i);
				found = true;
			}
		}

		if (!found)
		{
			cb->insertItem(cnt, opponent);
			cb->setCurrentIndex(cnt);
		}

		// check if it's me
		QString opp = opponent.section(' ', 0, 0);
		if (opp == myName)
		{
			IamPupil = true;
			set_Mode_real(modeMatch);
			win->getBoard()->set_player_colors (mv_counter % 2, !(mv_counter % 2));
			set_myColorIsBlack(mv_counter % 2);
		}
		else
		{
			IamPupil = false;
			set_Mode_real(modeObserve);
		}
	}
}

// teachtools - controls button has toggled
void qGoBoard::slot_ttControls(bool on)
{
	// teacher: on -> give controls to pupil
	//          off -> take controls back
	//		teacher has controls if BUTTON RELEASED (!ON)
	// pupil: only off -> give controls back
	//		pupil has controls if BUTTON PRESSED (ON)
	if (IamTeacher)
	{
		// check whether controls key has been clicked or has been toggled by command
		if ((haveControls && !on) || (!haveControls && on))
		{
			// toggled by command
		}
		else
		{
			if (on)
				client_window->sendcommand ("kibitz " + QString::number(id) + " #TC:ON", false);
			else
				client_window->sendcommand ("kibitz " + QString::number(id) + " #TC:OFF", false);

			haveControls = !on;
		}
	}
	else if (IamPupil)
	{
		// set game mode
		if (on)
		{
			// pupil is able to play both colors
			set_Mode_real (modeTeach);
		}
		else
		{
			// pupil has only one color
			set_Mode_real (modeMatch);
		}

		// check whether controls key has been clicked or has been toggled by command
		if ((!haveControls && !on) || (haveControls && on))
			// toggled by command
			return;

		client_window->sendcommand ("kibitz " + QString::number(id) + " #OC:OFF", false);
		win->pb_controls->setDisabled(true);
		haveControls = false;
	}
}

// teachtools - mark button has toggled
void qGoBoard::slot_ttMark(bool on)
{
	switch (gsName)
	{
		case IGS:
			// simple 'mark' cmd
			if (on)
				client_window->sendcommand ("mark", false);
			else
				client_window->sendcommand ("undo", false);
			break;

		case LGS:
			// use undo cmd
			if (on)
			{
				mark_set = true;
				mark_counter = mv_counter;
			}
			else
			{
				mark_set = false;
				for (; mark_counter <= mv_counter; mark_counter++)
					client_window->sendcommand ("undo", false);
				mark_counter = 0;
			}
			break;

		default:
			// use undo [n] cmd
			if (on)
			{
				mark_set = true;
				mark_counter = mv_counter;
			}
			else
			{
				mark_set = false;
				client_window->sendcommand ("undo " + QString::number(mv_counter - mark_counter + 1), false);
				mark_counter = 0;
			}
			break;
	}
}

