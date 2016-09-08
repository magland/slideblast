#ifndef SCREENSHOOTER_H
#define SCREENSHOOTER_H

#include <QObject>
#include <QString>
#include <QDebug>

class ScreenShooterPrivate;
class ScreenShooter : public QObject
{
    Q_OBJECT
public:
    friend class ScreenShooterPrivate;
    ScreenShooter();
    virtual ~ScreenShooter();
    void setCommand(QString cmd);
    void setWorkingDirectory(QString path);
    void setFramesPerSecond(double fps);
    void setDelay(double delay_sec);
    void start();
    void stop();
private slots:
    void slot_timer();
private:
    ScreenShooterPrivate *d;
};

#endif // SCREENSHOOTER_H
