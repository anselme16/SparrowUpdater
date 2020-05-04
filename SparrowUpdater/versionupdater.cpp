#include "versionupdater.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QVariantMap>
#include <QCryptographicHash>
#include <QProcess>

#include "updaterclient.h"

const QString tmpExe = "tmpExe";
const QString tmpData = "tmpData";

// =============== UTILITY ===============

static void parseDir(QString prefix, QDir dir, QStringList& paths)
{
    for(QString str : dir.entryList(QDir::Files))
        paths.push_back(prefix + str);
    for(QString str : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        parseDir(prefix + str + "/", QDir(dir.path() + '/' + str), paths);
};

static bool matchRegexpList(QString file, QStringList list)
{
    for(QString pattern : list)
        if(QRegExp(pattern).exactMatch(file))
            return true;
    return false;
};

static void computeHash(const QString &fileName, QByteArray& hash, quint64& size)
{
    QFile f(fileName);
    if(f.open(QFile::ReadOnly))
    {
        QByteArray data = f.readAll();
        QCryptographicHash hashFunc(QCryptographicHash::Sha1);
        size = data.size();
        hashFunc.addData(data);
        hash = hashFunc.result();
    }
}

static bool checkFile(const QString &fileName, const QByteArray& ref, const int& refSize)
{
    QFile f(fileName);
    if (f.open(QFile::ReadOnly))
    {
        QByteArray data = f.readAll();
        if(data.size() != refSize)
            return false;
        QCryptographicHash hashFunc(QCryptographicHash::Sha1);
        hashFunc.addData(data);
        return ref == hashFunc.result();
    }
    else
        return false;
}

// =============== VersionUpdater class ===============

VersionUpdater::VersionUpdater(QObject* parent, QString baseUrl)
:   QObject(parent)
,   _client(new UpdaterClient(this, baseUrl))
,   _currentStep(0)
{
    connect(_client, &UpdaterClient::receivedLastVersion, this, &VersionUpdater::handleVersion);
    connect(_client, &UpdaterClient::allFilesReceived, this, &VersionUpdater::handleFinished);
    connect(_client, &UpdaterClient::progressChanged, this, &VersionUpdater::progressChanged);
    connect(_client, &UpdaterClient::failed, this, [this](){ emit failure(_client->errors()); });
}

void VersionUpdater::updateApplication()
{
    // TODO
    // checking version
    // bool versionOk = version == qApp->applicationVersion();
}

QStringList VersionUpdater::parseAppFolder(QStringList whitelist, QStringList blacklist)
{
    static QStringList filepaths; // static so we only parse files once
    QDir appDir(qApp->applicationDirPath());
    if(filepaths.empty())
        parseDir(QString(), appDir, filepaths);
    
    QStringList filteredFilePaths;
    for(QString file : filepaths)
        if(matchRegexpList(file, whitelist) && !matchRegexpList(file, blacklist))
            filteredFilePaths << file;
    return filteredFilePaths;
}

void VersionUpdater::cleanupTmp()
{
    QDir tmpDir(qApp->applicationDirPath() + "/" + tmpExe);
    if(tmpDir.exists())
        tmpDir.removeRecursively();
}

QByteArray VersionUpdater::generateVersionJson(QStringList dataFiles, QStringList exeFiles)
{
    QDir appDir(qApp->applicationDirPath());
    
    // computing data hashes
    QByteArrayList dataHashs;
    QList<uint64_t> dataFileSizes;
    for(QString file : dataFiles)
    {
        QByteArray hash;
        quint64 size;
        computeHash(appDir.filePath(file), hash, size);
        if(hash.isEmpty())
            return QByteArray();
        dataHashs.push_back(hash);
        dataFileSizes.push_back(size);
    }
    
    // computing exe and dlls hashes
    QByteArrayList exeHashs;
    QList<uint64_t> exeFileSizes;
    for(QString file : exeFiles)
    {
        QByteArray hash;
        quint64 size;
        computeHash(appDir.filePath(file), hash, size);
        if(hash.isEmpty())
            return QByteArray();
        exeHashs.push_back(hash);
        exeFileSizes.push_back(size);
    }
    
    // generating the json
    QVariantMap map;
    map["version"] = qApp->applicationVersion();
    
    map["dataFiles"] = dataFiles;
    QVariantList dataHashList;
    for(QByteArray bytes : dataHashs)
        dataHashList.push_back(bytes.toBase64());
    map["dataHashs"] = dataHashList;
    QVariantList dataFileSizesList;
    for(quint64 size : dataFileSizes)
        dataFileSizesList.push_back(size);
    map["dataFileSizes"] = dataFileSizesList;
    
    map["exeFiles"] = exeFiles;
    QVariantList exeHashList;
    for(QByteArray bytes : exeHashs)
        exeHashList.push_back(bytes.toBase64());
    map["exeHashs"] = exeHashList;
    QVariantList exeFileSizesList;
    for(quint64 size : exeFileSizes)
        exeFileSizesList.push_back(size);
    map["exeFileSizes"] = exeFileSizesList;
    
    return QJsonDocument::fromVariant(map).toJson();
}

void VersionUpdater::getOnlineVersionInfo()
{
    _currentStep = 1;
    _client->getLastVersion();
}

void VersionUpdater::handleVersion(QByteArray versionJson)
{
    // Parsing json
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(versionJson, &jsonError);
    QString version;
    _remoteDataFiles    .clear();
    _remoteDataHashes   .clear();
    _remoteDataFileSizes.clear();
    _missingDataFiles   .clear();
    _remoteExeFiles     .clear();
    _remoteExeHashes    .clear();
    _remoteExeFileSizes .clear();
    _missingExeFiles    .clear();
    _currentStep = 2;
    if(jsonError.error == QJsonParseError::NoError)
    {
        QVariantMap map = doc.toVariant().toMap();
        version = map["version"].toString();
        
        _remoteExeFiles = map["exeFiles"].toStringList();
        for(QVariant v : map["exeHashs"].toList())
            _remoteExeHashes.push_back(QByteArray::fromBase64(v.toByteArray()));
        for(QVariant v : map["exeFileSizes"].toList())
            _remoteExeFileSizes.push_back(v.toInt());
        
        _remoteDataFiles = map["dataFiles"].toStringList();
        for(QVariant v : map["dataHashs"].toList())
            _remoteDataHashes.push_back(QByteArray::fromBase64(v.toByteArray()));
        for(QVariant v : map["dataFileSizes"].toList())
            _remoteDataFileSizes.push_back(v.toInt());
        
        emit onlineVersionReceived(version);
    }
    else
    {
        emit failure({"Json parsing error : " + jsonError.errorString()});
        return;
    }
}

bool VersionUpdater::checkFiles()
{
    bool filesOk = true;
 
    assert(_currentStep > 1); // try waiting for the onlineVersionReceived signal before calling this method
    
    if(_missingDataFiles.empty())
    {
        // checking data
        for(int i=0; i<_remoteDataFiles.size(); ++i)
        {
            QString filename = _remoteDataFiles[i];
            qint64 size = _remoteDataFileSizes[i];
            if(!checkFile(filename, _remoteDataHashes[i], size))
            {
                filesOk = false;
                _missingDataFiles.push_back({filename, size});
            }
        }
    }
    else
        filesOk = false;
    
    if(_missingExeFiles.empty())
    {
        // checking exe
        for(int i=0; i<_remoteExeFiles.size(); ++i)
        {
            QString filename = _remoteExeFiles[i];
            qint64 size = _remoteExeFileSizes[i];
            if(!checkFile(filename, _remoteExeHashes[i], _remoteExeFileSizes[i]))
            {
                filesOk = false;
                _missingExeFiles.push_back({filename, size});
            }
        }
    }
    else
        filesOk = false;
    
    return filesOk;
}

std::vector<std::pair<QString,qint64>> VersionUpdater::filesToUpdate()
{
    std::vector<std::pair<QString,qint64>> missingFiles;
    
    if(!checkFiles())
    {
        missingFiles.insert(missingFiles.begin(), _missingDataFiles.begin(), _missingDataFiles.end());
        missingFiles.insert(missingFiles.begin(), _missingExeFiles .begin(), _missingExeFiles .end());
    }
    
    return missingFiles;
}

bool VersionUpdater::restartRequired()
{
    checkFiles();
    return !_missingExeFiles.empty();
}

const std::unordered_map<QString, std::pair<qint64,qint64>>& VersionUpdater::getDetailedProgress()
{
    return _client->getDetailedProgress();
}

std::pair<qint64,qint64> VersionUpdater::getTotalProgress()
{
    return _client->getTotalProgress();
}

void VersionUpdater::downloadFiles()
{
    checkFiles();
    for(auto it : _missingDataFiles)
        _client->getFile(it.first, tmpData);
    for(auto it : _missingExeFiles)
        _client->getFile(it.first, tmpExe);
}

void VersionUpdater::handleFinished()
{
    _currentStep = 3;
    emit allFilesDownloaded();
}

bool VersionUpdater::applyDataPatch()
{
    QString source = qApp->applicationDirPath() + '/' + tmpData + '/';
    QString target = qApp->applicationDirPath() + '/';
    
    for(auto it : _missingDataFiles)
    {
        const QString& filename = it.first;
        if(!QFileInfo::exists(source + filename))
        {
            emit failure({tr("Source file doesn't exist : %1").arg(source + filename)});
            return false;
        }
        if(QFileInfo::exists(target + filename))
        {
            if(!QFile::remove(target + filename))
            {
                emit failure({tr("Can't overwrite file : %1").arg(target + filename)});
                return false;
            }
        }
        if(!QFile::copy(source + filename, target + filename))
        {
            emit failure({tr("Can't create file : %1").arg(target + filename)});
            return false;
        }
    }
    return true;
}

bool VersionUpdater::applyExePatchAndRestart()
{
    assert(_currentStep == 3); // try waiting for all files to be downloaded before calling this method
    if(restartRequired())
    {
        QString source = qApp->applicationDirPath() + '/' + tmpExe + '/';
        for(auto it : _missingExeFiles) // check if files have been successfully downloaded
        {
            const QString& filename = it.first;
            if(!QFileInfo::exists(source + filename))
            {
                emit failure({tr("Source file doesn't exist : %1").arg(source + filename)});
                return false;
            }
        }
        
        QFile batFile(source + "updater.bat");
        if(!batFile.open(QFile::WriteOnly | QFile::Text))
        {
            emit failure({tr("cannot write update script : %1").arg(source + "updater.bat")});
            return false;
        }
        QString text =
          ":copyit"
          "timeout /t 1" // wait 1 second
          "xcopy " + tmpExe + " . /Y /E /I" // try copying tmp folder into app folder
          "IF %errorlevel% NEQ 0 GOTO copyit" // if copying failed try again
          "start " + qApp->arguments().at(1); // start the application
        batFile.write(text.toUtf8());
        batFile.close();
        return QProcess::startDetached(QString("cmd /c " + tmpExe + "/updater.bat"));
    }
    return false; // nothing to do
}
