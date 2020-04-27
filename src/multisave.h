#include <memory>

#include <QDialog>

class game_record;
typedef std::shared_ptr<game_record> go_game_ptr;
class game_state;
class QButtonGroup;

namespace Ui
{
	class MultiSaveDialog;
};

class MultiSaveDialog : public QDialog
{
	Ui::MultiSaveDialog *ui;

	go_game_ptr m_game;
	game_state *m_position;
	QButtonGroup *m_radios;
	int m_count;

	bool save_one (const QString &, int);
	void do_save ();
	void update_count (bool = false);
	void choose_file ();

public:
	MultiSaveDialog (QWidget *, go_game_ptr, game_state *);
	~MultiSaveDialog ();
};
