#include <memory>

#include <QDialog>

class game_record;
typedef std::shared_ptr<game_record> go_game_ptr;
class game_state;
class QButtonGroup;
class an_id_model;
class MainWindow;

namespace Ui
{
	class EditAnalysisDialog;
};

class EditAnalysisDialog : public QDialog
{
	Ui::EditAnalysisDialog *ui;

	MainWindow *m_parent;
	go_game_ptr m_game;
	an_id_model *m_model;

	void update_buttons ();
	void do_delete (bool);
public:
	EditAnalysisDialog (MainWindow *, go_game_ptr, an_id_model *);
	~EditAnalysisDialog ();
};
