#include "screenshooter.h"

#include <QProcess>
#include <QTime>
#include <QTimer>

class ScreenShooterPrivate
{
public:
    ScreenShooter *q;
    QTime m_timer;
    QString m_command;
    double m_fps=1;
    int m_num_screenshots_taken=0;
    bool m_shooting=false;
    double m_delay=0; //sec

    void take_screenshot();
};

ScreenShooter::ScreenShooter() {
    d=new ScreenShooterPrivate;
    d->q=this;
    QTimer::singleShot(0,this,SLOT(slot_timer()));
}

ScreenShooter::~ScreenShooter() {
    delete d;
}

void ScreenShooter::setCommand(QString cmd)
{
    d->m_command=cmd;
}

void ScreenShooter::setFramesPerSecond(double fps)
{
    d->m_fps=fps;
}

void ScreenShooter::setDelay(double delay_sec)
{
    d->m_delay=delay_sec;
}

void ScreenShooter::start()
{
    d->m_timer.restart();
    d->m_num_screenshots_taken=0;
    d->m_shooting=true;
}

void ScreenShooter::stop()
{
    if (d->m_num_screenshots_taken==0) {
        d->take_screenshot(); //take at least one screenshot
    }
    d->m_shooting=false;
}

void ScreenShooter::slot_timer()
{
    if (d->m_shooting) {
        if (d->m_num_screenshots_taken<d->m_fps*(d->m_timer.elapsed()*1.0/1000)) {
            d->take_screenshot();
            d->m_num_screenshots_taken++;
            qDebug() << QString("%1 shots taken in %2 seconds. That's %3 shots per sec. Compare with fps=%4").arg(d->m_num_screenshots_taken).arg(d->m_timer.elapsed()*1.0/1000).arg(d->m_num_screenshots_taken/(d->m_timer.elapsed()*1.0/1000)).arg(d->m_fps);
            qDebug() << d->m_num_screenshots_taken << d->m_fps*(d->m_timer.elapsed()*1.0/1000);
        }
    }
    QTimer::singleShot(100,this,SLOT(slot_timer()));
}

void ScreenShooterPrivate::take_screenshot()
{
    qDebug() << "Screenshot: "+m_command;
    if (system(m_command.toLatin1().data())!=0) {
        qWarning() << "Non-zero exit code for screenshot";
    }
}
