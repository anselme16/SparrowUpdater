#include "updaterclient.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QAuthenticator>
#include <QFile>
#include <QDir>
#include <QCoreApplication>

static const QString VERSION_FILE = "version.json";

UpdaterClient::UpdaterClient(QObject* parent, const QString &baseUrl)
:	QObject(parent)
,   nbFilesPending(0)
,   _baseUrl(baseUrl)
,   _hasFailed(false)
{
	manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::authenticationRequired            , this, [this](){_hasFailed = true; _errors << "QNetworkAccessManager::authenticationRequired";});
    connect(manager, &QNetworkAccessManager::preSharedKeyAuthenticationRequired, this, [this](){_hasFailed = true; _errors << "QNetworkAccessManager::preSharedKeyAuthenticationRequired";});
    connect(manager, &QNetworkAccessManager::proxyAuthenticationRequired       , this, [this](){_hasFailed = true; _errors << "QNetworkAccessManager::proxyAuthenticationRequired";});
    connect(manager, &QNetworkAccessManager::networkAccessibleChanged          , this, [this](){_hasFailed = true; _errors << "QNetworkAccessManager::networkAccessibleChanged";});
    connect(manager, &QNetworkAccessManager::sslErrors                         , this, [this](){_hasFailed = true; _errors << "QNetworkAccessManager::sslErrors";});
    connect(manager, &QNetworkAccessManager::encrypted                         , this, [this](){_hasFailed = true; _errors << "QNetworkAccessManager::encrypted";});
}

void UpdaterClient::getLastVersion()
{
    QNetworkRequest request(QUrl(_baseUrl + VERSION_FILE));
    QNetworkReply* reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [=](){ handleVersion(reply); });
}

void UpdaterClient::getFile(QString filename, QString dstDir)
{
    _progress[filename] = {0,0};
    
    QNetworkRequest request(QUrl(_baseUrl + filename));
    QNetworkReply* reply = manager->get(request);
    connect(reply, &QNetworkReply::downloadProgress, this, [=](qint64 bytesReceived, qint64 bytesTotal){
        _progress[filename] = {bytesReceived, bytesTotal};
        emit progressChanged();
    });
    connect(reply, &QNetworkReply::finished, this, [=](){ handleFile(reply, filename, dstDir); });
    connect(this, &UpdaterClient::failed, reply, &QNetworkReply::abort);
    ++nbFilesPending;
}

void UpdaterClient::handleVersion(QNetworkReply* reply)
{
    reply->deleteLater();
    if(reply->error() != QNetworkReply::NetworkError::NoError)
    {
        _hasFailed = true;
        _errors << reply->errorString();
        emit failed();
    }
    else
    {
        _hasFailed = false;
        emit receivedLastVersion(reply->readAll());
    }
}

void UpdaterClient::handleFile(QNetworkReply* reply, QString filename, QString dstDir)
{
    const auto customAssert = [this](bool condition, QString msg){
        if(!condition)
        {
            _errors << msg;
            _hasFailed = true;
        }
    };
    
    reply->deleteLater();
    if(!_hasFailed)
    {
        if(reply->error() != QNetworkReply::NetworkError::NoError)
        {
            _hasFailed = true;
            _errors << reply->errorString();
            emit failed();
        }
        else
        {
            filename = dstDir + "/" + filename;
            QFileInfo info(filename);
            QString folder = info.dir().path();
            customAssert(QDir(qApp->applicationDirPath()).mkpath(folder), QString("Can't create %1 dir").arg(folder));
            QFile file(qApp->applicationDirPath() + '/' + filename);
            customAssert(file.open(QFile::WriteOnly),QString("Can't open file for writing : %1").arg(filename));
            QByteArray data = reply->readAll();
            customAssert(file.write(data) == data.size(), QString("Failed to write %1 bytes in file : %2").arg(data.size()).arg(filename));
            // emit fileReceived(filename);
            if(--nbFilesPending == 0)
                emit allFilesReceived();
            if(_hasFailed)
                emit failed();
        }
    }
    if(_hasFailed)
        _progress.erase(filename);
}

std::pair<qint64,qint64> UpdaterClient::getTotalProgress()
{
    qint64 progress = 0;
    qint64 total = 0;
    for(auto it : _progress)
    {
        progress += it.second.first;
        total += it.second.second;
    }
    return {progress, total};
}




