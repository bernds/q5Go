#include <array>

#include "config.h"
#include "setting.h"
#include "tips.h"
#include "ui_tips_gui.h"

namespace {
struct totd {
	QString icon_name;
	QString text;
};

std::array<totd, 9> tips {
	totd { ":/totd/images/totd/docks.png",
		QObject::tr ("<p>You can configure the layout of the board window by dragging the docks to the position you like. "
			     "You can then save the layout in the View menu. New board windows try to restore the most appropriate"
			     "layout, and you can also press L to restore a previously saved layout.</p>") },
	totd { ":/totd/images/totd/multi.png",
		QObject::tr ("<p>When observing online matches, you can choose whether to view them in individual windows, or in "
			     "grouped together in a single window.</p>"
			     "<p>Use the icon from the client window's toolbar, highlighted in the screenshot, to toggle the behaviour.</p>") },
	totd { ":/totd/images/totd/anvars.png",
		QObject::tr ("<p>When analyzing a game, variations displayed on the board can be permanently added to the game "
			     "record by shift-clicking on them.</p>") },
	totd { ":/totd/images/totd/ctrlclick.png",
		QObject::tr ("<p>To quickly go to the position where a stone was played on the board, hold Ctrl and click on it.</p>") },
	totd { ":/totd/images/totd/tut.png",
		QObject::tr ("<p>If you are a beginner, choose the \"Learn Go\" button in the greeter dialog, or one of the menu items "
			     "in other windows, to bring up a window with multiple tutorials on how to play Go.</p>") },
	totd { ":/totd/images/totd/multieval.png",
		QObject::tr ("<p>You can analyze a game with multiple different engines. Winrate graphs are shown together, while "
			     "score graphs can be selected individually.</p><p>There is also a menu item to remove unwanted older "
			     "analysis from a file.</p>") },
	totd { ":/totd/images/totd/visual.png",
		QObject::tr ("<p>Beginners may find it helpful to use the \"Visualize Connections\" option "
			     "from the View menu. This shows which stones are connected to each other.</p>"
			     "<p>Please try not to rely on this for long - it is not available in match play, or on a real Go board. "
			     "It is intended for absolute beginners only.</p>") },
	totd { ":/totd/images/totd/evalmodes.png",
		QObject::tr ("<p>Using the right-click menu, you can change the evaluation graph to show winrates or scores "
			     "(assuming you are using an engine like KataGo which reports both).</p>") },
	totd { ":/totd/images/totd/collapse.png",
		QObject::tr ("<p>When working with a file that has many variations, you can use the right-click menu "
			     "on the game tree to collapse branches into a single box.</p>"
			     "<p>There is also an auto-collapse mode, where only the line containing the current position "
			     "is shown fully expanded and everything else is hidden.</p>") }
};
}

void TipsDialog::set_tip ()
{
	m_idx %= tips.size ();
	ui->imgLabel->setPixmap (QPixmap (tips[m_idx].icon_name));
	ui->textLabel->setText (tips[m_idx].text);
	setting->writeIntEntry ("TIPS_IDX", m_idx + 1);
}

TipsDialog::TipsDialog (QWidget* parent)
	: QDialog (parent), ui (new Ui::TipsDialog)
{
	ui->setupUi (this);

	m_idx = setting->readIntEntry ("TIPS_IDX");
	if (m_idx < 0)
		m_idx = 0;
	set_tip ();
	ui->startupCheckBox->setChecked (setting->readBoolEntry ("TIPS_STARTUP"));

	leftAction = new QAction (this);
	leftAction->setShortcut (Qt::Key_Left);
	connect (leftAction, &QAction::triggered, [this] (bool)
		 {
			 m_idx = m_idx + tips.size () - 1;
			 set_tip ();
		 });
	rightAction = new QAction (this);
	rightAction->setShortcut (Qt::Key_Right);
	connect (rightAction, &QAction::triggered, [this] (bool)
		 {
			 m_idx++;
			 set_tip ();
		});
	connect (ui->prevButton, &QToolButton::clicked, [this] (bool) { leftAction->trigger (); });
	connect (ui->nextButton, &QToolButton::clicked, [this] (bool) { rightAction->trigger (); });
	connect (ui->startupCheckBox, &QCheckBox::toggled, [] (bool on)
		 {
			 setting->writeBoolEntry ("TIPS_STARTUP", on);
		 });
	addAction (leftAction);
	addAction (rightAction);
}

TipsDialog::~TipsDialog ()
{
	delete leftAction;
	delete rightAction;
}
