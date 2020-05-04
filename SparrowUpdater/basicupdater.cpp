#include "basicupdater.h"

#include "versionupdater.h"

#include <QCoreApplication>

BasicUpdater::BasicUpdater(QObject *parent, QString baseUrl)
:   QObject(parent)
,   _updater(new VersionUpdater(this, baseUrl))
{
    connect(_updater, &VersionUpdater::failure, this, &BasicUpdater::failure);
    connect(_updater, &VersionUpdater::progressChanged, this, [this](){
        auto progress = _updater->getTotalProgress();
        emit progressChanged(progress.first, progress.second);
    });
}

void BasicUpdater::updateApplication()
{
    emit progressChanged(0,0);
    
    _updater->getOnlineVersionInfo();
    
    connect(_updater, &VersionUpdater::onlineVersionReceived, this, [this](QString version){
        if(!_updater->checkFiles())
            _updater->downloadFiles();
        else
            emit success();
    });
    
    connect(_updater, &VersionUpdater::allFilesDownloaded, this, [this](){
        _updater->applyDataPatch();
        if(_updater->restartRequired())
        {
            if(_updater->applyExePatchAndRestart())
                qApp->quit();
        }
        else
            emit success();
    });
}
