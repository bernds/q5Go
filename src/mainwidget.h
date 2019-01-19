#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include "misctools.h"
#include "ui_mainwidget_gui.h"
#include "setting.h"
#include "defines.h"

class go_board;
class MainWindow;

class MainWidget : public QWidget, public Ui::MainWidgetGui
{
	Q_OBJECT

public:
	MainWidget(MainWindow *, QWidget* parent = 0);
	~MainWidget();

	void toggleSlider(bool);
	bool getSlider() { return showSlider; }
	void toggleSliderSignal(bool b) { sliderSignalToggle = b; }
	void updateFont();
	void setToolsTabWidget(enum tabType=tabNormalScore, enum tabState=tabSet);
	void setGameMode (GameMode);

	void toggleSidebar (bool);
	void setSliderMax(int n);
	void setCaptures(float black, float white, bool scored=false);
	void setTimes(const QString &btime, const QString &bstones,
		      const QString &wtime, const QString &wstones,
		      bool warn_b, bool warn_w, int);
#if 0
	void setTimes(bool, float, int);
#endif

	void update_game_record (std::shared_ptr<game_record>);
	void init_game_record (std::shared_ptr<game_record>);
	void setMoveData(const game_state &, const go_board &, GameMode);
	void recalc_scores (const go_board &);

	void grey_eval_bar ();
	void set_eval (double);
	void set_eval (const QString &, double, stone_color, int);
	void set_2nd_eval (const QString &, double, stone_color, int);
public slots:
	void on_colorButton_clicked(bool);
	void slot_toolsTabChanged(int);
	virtual void doRealScore(bool);
	virtual void doEdit();
	virtual void doEditPos(bool);
	virtual void sliderChanged(int);

private:
	std::shared_ptr<game_record> m_game;
	bool showSlider, sliderSignalToggle;
	MainWindow *m_mainwin;
	GameMode m_remember_mode;
	int m_remember_tab;
	QGraphicsScene *m_eval_canvas;
	QGraphicsRectItem *m_eval_bar;
	QGraphicsTextItem *m_w_time, *m_b_time;
	double m_eval;
};

#endif
