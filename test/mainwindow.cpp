#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QProgressDialog>
#include <QFile>

#include "SparrowUpdater/versionupdater.h"

// normal dialog flags, without the useless "?" button
const Qt::WindowFlags defaultDialogFlags = Qt::Dialog | Qt::WindowTitleHint | Qt::WindowSystemMenuHint;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->statusbar->addPermanentWidget(new QLabel(qApp->applicationVersion()));
    _updater = new VersionUpdater(this, "http://localhost:8000/");
    
    _progress = new QProgressDialog(tr("Checking for updates..."), tr("Cancel"), 0, 0, this, defaultDialogFlags);
    _progress->setModal(true);
    _progress->setWindowTitle(tr("Updater"));
    _progress->show();
    _progress->setFocus();
    
    QObject::connect(_updater, &VersionUpdater::failure,         this, &MainWindow::onError);
    QObject::connect(_updater, &VersionUpdater::progressChanged, this, &MainWindow::onProgress);
    
    // default updater
    // _updater->updateApplication();
    
    // customized updater
    customUpdate();
}

MainWindow::~MainWindow()
{
}

void MainWindow::onError(QStringList errors)
{
    QString msg = tr("couldn't check for updates :");
    for(QString err : errors)
        msg += '\n' + err;
    QMessageBox::critical(this, "info", msg);
    finished(false);
}

void MainWindow::onProgress()
{
    auto progress = _updater->getTotalProgress();
    _progress->setRange(0, progress.second);
    _progress->setValue(progress.first);
}

void MainWindow::customUpdate()
{
    _updater->cleanupTmp();
    
    _updater->getOnlineVersionInfo();
    
    connect(_updater, &VersionUpdater::onlineVersionReceived, this, [this](QString version){
        bool getUpdate = false;
        
        if(!_updater->checkFiles())
        {
            if(!_updater->restartRequired())
            {
                // don't ask for confirmation for minor update
                getUpdate = true;
            }
            else
            {
                // ask for confirmation
                qint64 size = 0;
                for(auto it : _updater->filesToUpdate())
                    size += it.second;
                QString infoStr = tr("An update is available %1 => %2 \nDo you want to download the update ? (size = %3 bytes)");
                infoStr = infoStr.arg(qApp->applicationVersion()).arg(version).arg(size);
                getUpdate = (QMessageBox::question(nullptr, tr("Update available"), infoStr) == QMessageBox::Yes);
            }
        }
        
        if(getUpdate)
        {
            _updater->downloadFiles();
            _progress->setLabelText(tr("Downloading update..."));
        }
        else
            finished(true);
    });
    
    connect(_updater, &VersionUpdater::allFilesDownloaded, this, [this](){
        _updater->applyDataPatch();
        if(_updater->restartRequired())
        {
            if(_updater->applyExePatchAndRestart())
            {
                QMessageBox::information(this,tr("Application will restart"),tr("To finish the update, the application will now restart."));
                close();
            }
        }
        else
            finished(true);
    });
}

void MainWindow::finished(bool ok)
{
    setEnabled(true);
    _progress->hide();
    if(ok)
    {
        ui->statusbar->showMessage(tr("App up to date"));
        
        QFile testData(qApp->applicationDirPath() + "/data/test.json");
        testData.open(QFile::ReadOnly);
        ui->data->setPlainText(testData.readAll());
    }
    else
    {
        // do whatever you want when app cannot update
        ui->statusbar->showMessage(tr("Failed to update, Offline mode enabled"));
    }
}
