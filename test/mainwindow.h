#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>

namespace Ui {
class MainWindow;
}
class QProgressDialog;
class VersionUpdater;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
public slots:
    void onError(QStringList errors);
    void onProgress();
    
private:
    void customUpdate();
    void finished(bool ok);
    
    std::unique_ptr<Ui::MainWindow> ui;
    VersionUpdater* _updater;
    QProgressDialog* _progress;
};

#endif // MAINWINDOW_H
