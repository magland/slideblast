#ifndef SBMAINWINDOW_H
#define SBMAINWINDOW_H

#include <QWidget>

class SBMainWindowPrivate;
class SBMainWindow : public QWidget
{
    Q_OBJECT
public:
    friend class SBMainWindowPrivate;
    SBMainWindow();
    virtual ~SBMainWindow();
private slots:
    void slot_start_recording();
    void slot_stop_recording();
    void slot_save_recording();
private slots:
    void slot_update_buttons_and_info();
private:
    SBMainWindowPrivate *d;
};

#endif // SBMAINWINDOW_H
