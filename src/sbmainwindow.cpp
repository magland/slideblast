#include "sbmainwindow.h"

#include <QDateTime>
#include <QDir>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QProcess>
#include <QPushButton>
#include <QVBoxLayout>
#include <QCoreApplication>
#include <QFileDialog>
#include <QInputDialog>
#include <QTimer>
#include "screenshooter.h"
#include <QMessageBox>

class SBMainWindowPrivate
{
public:
    SBMainWindow *q;
    QGridLayout *m_glayout;
    QHBoxLayout *m_button_layout;
    QLabel *m_info;
    bool m_is_recording=false;
    bool m_recorded_something=false;
    QString m_working_directory;
    ScreenShooter m_screen_shooter;
    int m_current_segment_number=0;

    QMap<QString,QLineEdit*> m_line_edits;
    QMap<QString,QAbstractButton*> m_buttons;

    QProcess *m_audio_recording_process=0;

    void add_parameter(QString name, QString label, QVariant val);
    void add_button(QString name,QString label,QObject *receiver,const char *signal_or_slot);

    QVariant get_parameter_value(QString name);

    void update_buttons_and_info();
    QString preprocess_command(QString cmd,int segment_number);
    QString make_working_directory();
    void delete_processes();
    void clean_up_working_directory();
    QString working_directory(int segment_number);
    void set_info(QString str);
};

SBMainWindow::SBMainWindow() {
    d=new SBMainWindowPrivate;
    d->q=this;

    d->m_working_directory=d->make_working_directory();

    d->m_glayout=new QGridLayout;
    d->m_button_layout=new QHBoxLayout;
    d->m_info=new QLabel;

    d->add_button("start_recording","Start recording",this,SLOT(slot_start_recording()));
    d->add_button("stop_recording","Stop recording",this,SLOT(slot_stop_recording()));
    d->add_button("save_recording","Save recording",this,SLOT(slot_save_recording()));

    d->add_parameter("audio_recording_command","Audio recording command","arecord -f S16_LE -r 24000 -d 0 $AUDIO_FILE$");
    d->add_parameter("screen_capture_command","Screen capture command","shutter -a --output=$WORKDIR$/shutter-%Y-%m-%d-%T.jpg -e");
    d->add_parameter("resize_images_command","Resize images command","convert '$WORKDIR$/*.jpg' -resize 2000x1200 -background black -gravity center -extent 2000x1200 $WORKDIR$/resized%03d.jpg");
    d->add_parameter("build_movie_command","Build movie command","ffmpeg -framerate $FPS$ -pattern_type glob -i '$WORKDIR$/resized*.jpg' -i $AUDIO_FILE$ -c:v libx264 -r 10 -shortest -strict -2 $OUTPUT_FILE$");
    d->add_parameter("concat_movie_command","Concat movie command","ffmpeg -f concat -i $INPUT_TXT_FILE$ -codec copy $FINAL_OUTPUT_FILE$");
    d->add_parameter("fps","Frames per second","0.25");
    d->add_parameter("screenshot_delay","Screenshot delay (sec)","0");

    QVBoxLayout *vlayout=new QVBoxLayout;
    this->setLayout(vlayout);

    d->m_button_layout->addStretch();
    vlayout->addLayout(d->m_glayout);
    vlayout->addLayout(d->m_button_layout);
    vlayout->addWidget(d->m_info);

    this->setMinimumWidth(1000);

    d->update_buttons_and_info();
}

SBMainWindow::~SBMainWindow() {
    slot_stop_recording();
    d->clean_up_working_directory();
    delete d;
}

void SBMainWindow::slot_start_recording()
{
    if (d->m_is_recording) return;
    d->m_recorded_something=true;
    d->m_current_segment_number++;

    //sometimes we have trouble with the first part of the recording when concatenating segments

    d->delete_processes();

    {
        d->m_audio_recording_process=new QProcess;
        QString cmd=d->get_parameter_value("audio_recording_command").toString();
        cmd=d->preprocess_command(cmd,d->m_current_segment_number);
        d->m_audio_recording_process->start(cmd);
    }

    {
        double fps=d->get_parameter_value("fps").toDouble();
        double screenshot_delay=d->get_parameter_value("screenshot_delay").toDouble();
        QString cmd=d->get_parameter_value("screen_capture_command").toString();
        cmd=d->preprocess_command(cmd,d->m_current_segment_number);
        d->m_screen_shooter.stop();
        d->m_screen_shooter.setCommand(cmd);
        d->m_screen_shooter.setFramesPerSecond(fps);
        d->m_screen_shooter.setDelay(screenshot_delay);
        d->m_screen_shooter.start();
    }

    d->m_is_recording=true;
    d->update_buttons_and_info();
    d->set_info("Starting.... please wait....");
    QTimer::singleShot(2000,this,SLOT(slot_update_buttons_and_info()));
}

void SBMainWindow::slot_stop_recording()
{
    if (!d->m_is_recording) return;
    if (!d->m_audio_recording_process) return;

    d->m_audio_recording_process->kill();
    d->m_audio_recording_process->waitForFinished();
    d->delete_processes();
    d->m_screen_shooter.stop();
    d->m_is_recording=false;
    d->update_buttons_and_info();
}

void write_text_file(const QString& path, const QString& txt)
{
    QFile f(path);
    if (!f.open(QFile::WriteOnly | QFile::Text))
        return;
    QTextStream out(&f);
    out << txt;
}

void repl(QString &str,QString X,QString Y) {
    str=str.split(X).join(Y);
}

void SBMainWindow::slot_save_recording()
{
    QString save_path=QFileDialog::getExistingDirectory(0,"Save recording to directory");
    if (save_path.isEmpty()) return;

    QString recording_name=QInputDialog::getText(0,"Save recording","Enter a name for the recording");
    if (recording_name.isEmpty()) return;

    QDir(save_path).mkpath(recording_name);

    for (int s=1; s<=d->m_current_segment_number; s++) {
        QString movie0=d->working_directory(s)+"/output.mp4";
        QString info0=QString("Saving segment %1/%2").arg(s).arg(d->m_current_segment_number);
        d->set_info(info0);
        { //resize
            d->set_info(info0+": resizing");
            QString cmd=d->get_parameter_value("resize_images_command").toString();
            cmd=d->preprocess_command(cmd,s);
            qDebug() << "Resizing images: "+cmd;
            if (system(cmd.toLatin1().data())!=0) {
                qWarning() << "Resize images command returned non-zero exit code";
            }
        }

        { //build
            d->set_info(info0+": building movie");
            QString cmd=d->get_parameter_value("build_movie_command").toString();
            cmd=d->preprocess_command(cmd,s);
            qDebug() << "Building movie: "+cmd;
            if (system(cmd.toLatin1().data())!=0) {
                qWarning() << "Build movie command returned non-zero exit code";
            }
        }

        if (!QFile::exists(movie0)) {
            QMessageBox::information(0,"Unable to create movie","Something went wrong. See the console for more information.");
            return;
        }

        d->set_info(info0+": moving files");
        QString new_movie_path=QString("%1/%2/%3-seg-%4.mp4").arg(save_path).arg(recording_name).arg(recording_name).arg(s);
        if (!QFile::rename(movie0,new_movie_path)) {
            QMessageBox::information(0,"Unable to create movie","Something went wrong. Unable to move file to: "+new_movie_path);
            return;
        }
        QDir(save_path).mkpath(QString("%1/%2-images-%3").arg(recording_name).arg(recording_name).arg(s));
        QStringList list=QDir(d->working_directory(s)).entryList(QStringList("*"),QDir::Files);
        foreach (QString str,list) {
            if (!str.startsWith("resize")) {
                QString new_image_fname=QString("%1/%2/%3-images-%4/%5").arg(save_path).arg(recording_name).arg(recording_name).arg(s).arg(str);
                if (!QFile::rename(d->working_directory(s)+"/"+str,new_image_fname)) {
                    QMessageBox::information(0,"Unable to create movie","Something went wrong. Unable to move image to :"+new_image_fname);
                    return;
                }
            }
        }
    }

    {
        d->set_info("Concatenating output");
        QString txt0;
        for (int s=1; s<=d->m_current_segment_number; s++) {
            txt0+=QString("file '%1/%2/%3-seg-%4.mp4'\n").arg(save_path).arg(recording_name).arg(recording_name).arg(s);
        }
        QString input_txt_fname=QString("%1/%2/input.txt").arg(save_path).arg(recording_name);
        write_text_file(input_txt_fname,txt0);

        QString cmd=d->get_parameter_value("concat_movie_command").toString();
        QString final_output_path=QString("%1/%2/%3.mp4").arg(save_path).arg(recording_name).arg(recording_name);
        repl(cmd,"$FINAL_OUTPUT_FILE$",final_output_path);
        repl(cmd,"$INPUT_TXT_FILE$",input_txt_fname);
        qDebug() << "Concatenating movies: "+cmd;
        if (system(cmd.toLatin1().data())!=0) {
            qWarning() << "Concat movie command returned non-zero exit code";
        }
    }


    d->clean_up_working_directory();
    d->m_current_segment_number=0;
    d->update_buttons_and_info();
}

void SBMainWindow::slot_update_buttons_and_info()
{
    d->update_buttons_and_info();
}

void SBMainWindowPrivate::add_parameter(QString name, QString label, QVariant val)
{
    QLineEdit *X=new QLineEdit;
    X->setText(val.toString());
    m_line_edits[name]=X;

    int r=m_glayout->rowCount();
    m_glayout->addWidget(new QLabel(label),r,0);
    m_glayout->addWidget(X,r,1);
}

void SBMainWindowPrivate::add_button(QString name, QString label,QObject *receiver,const char *signal_or_slot)
{
    QPushButton *B=new QPushButton(label);
    m_button_layout->addWidget(B);
    m_buttons[name]=B;
    QObject::connect(B,SIGNAL(clicked(bool)),receiver,signal_or_slot);
}

QVariant SBMainWindowPrivate::get_parameter_value(QString name)
{
    if (m_line_edits.contains(name)) {
        return m_line_edits[name]->text();
    }
    else return QVariant();
}

void SBMainWindowPrivate::update_buttons_and_info()
{
    m_buttons["start_recording"]->setEnabled(!m_is_recording);
    m_buttons["stop_recording"]->setEnabled(m_is_recording);
    m_buttons["save_recording"]->setEnabled((!m_is_recording)&&(m_recorded_something));

    QString info;
    if (m_is_recording) {
        info=QString("Recording segment %1.").arg(m_current_segment_number);
    }
    else {
        info=QString("Recorded %1 segments.").arg(m_current_segment_number);
    }
    m_info->setText(info);
}



QString SBMainWindowPrivate::preprocess_command(QString cmd,int segment_number)
{
    repl(cmd,"$WORKDIR$",working_directory(segment_number));
    repl(cmd,"$AUDIO_FILE$",working_directory(segment_number)+"/audio.wav");
    repl(cmd,"$OUTPUT_FILE$",working_directory(segment_number)+"/output.mp4");
    repl(cmd,"$FPS$",get_parameter_value("fps").toString());
    return cmd;
}

QString SBMainWindowPrivate::make_working_directory()
{
    QString basedir=qApp->applicationDirPath()+"/..";
    QString timestamp=QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    QString path0=QString("tmp/%1").arg(timestamp);
    QDir(basedir).mkpath(path0);
    return basedir+"/"+path0;
}

void SBMainWindowPrivate::delete_processes()
{
    if (m_audio_recording_process) delete m_audio_recording_process;
    m_audio_recording_process=0;

}

void SBMainWindowPrivate::clean_up_working_directory()
{
    if (!m_working_directory.contains("/tmp/")) {
        qWarning() << "Unexpected that working directory does not contain /tmp/. Not cleaning up :"+m_working_directory;
    }
    QStringList list1=QDir(m_working_directory).entryList(QStringList("*"),QDir::Files);
    foreach (QString str1,list1) {
        QFile::remove(m_working_directory+"/"+str1);
    }
    QStringList list2=QDir(m_working_directory).entryList(QStringList("*"),QDir::Dirs|QDir::NoDotAndDotDot);
    foreach (QString str2,list2) {
        QStringList list3=QDir(m_working_directory+"/"+str2).entryList(QStringList("*"),QDir::Files);
        foreach (QString str3,list3) {
            QFile::remove(m_working_directory+"/"+str2+"/"+str3);
        }
        QDir(m_working_directory).rmdir(str2);
    }
    if (!QDir(QFileInfo(m_working_directory).path()).rmdir(QFileInfo(m_working_directory).fileName())) {
        qWarning() << "Unable to clean up working directory: "+m_working_directory;
    }
}

QString SBMainWindowPrivate::working_directory(int segment_number)
{
    QString tmp=QString("seg-%1").arg(segment_number);
    QDir(m_working_directory).mkpath(tmp);
    return m_working_directory+"/"+tmp;
}

void SBMainWindowPrivate::set_info(QString str)
{
    m_info->setText(str);
    m_info->repaint();
    qApp->processEvents();
}

