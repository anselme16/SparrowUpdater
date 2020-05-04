#ifndef VERSIONUPDATER_H
#define VERSIONUPDATER_H

#include <QObject>

class UpdaterClient;

#ifndef QSTRING_HASH
#define QSTRING_HASH

#include <unordered_map>
#include <QHash>
#include <functional>

namespace std {
  template<> struct hash<QString> {
    std::size_t operator()(const QString& s) const noexcept {
      return (size_t) qHash(s);
    }
  };
}
#endif // QSTRING_HASH

/**
 * @brief The VersionUpdater class handles auto-updating based on file hashes and version numbers
 * 
 * limitations : currently, this updater is not capable of deleting files, only adding files or overriding them
 */
class VersionUpdater : public QObject
{
    Q_OBJECT
public:
    /**
     * Constructor
     * baseUrl is the http URL the updates will be pulled from
     */
    VersionUpdater(QObject* parent = nullptr, QString baseUrl = "http://localhost/");
    
signals:
    /**
     * signal received when the VersionUpdater encounters an error
     * it can contain:
     * - network errors (mostly if the server can't be reached or a file is missing)
     * - json parsing errors (if you version.json is crap, you can use "generateVersionJson")
     * - filesystem errors (mostly if your app doesn't have the rights to overwrite its own files)
     * 
     * if you want to retry the updating process after a failure, I recommend starting back from step 1
     */
    void failure(QStringList errors);
    
    // ====================  STEP 1  ========================
    
public slots:
    
    /** 
     * first method to call, this requests latest version information from the server
     * listen the "onlineVersionReceived" signal to get the answer to this network request
     */
    void getOnlineVersionInfo();
    
signals:
    
    /**
     * emitted when the online version information has been received
     * version contains the online version string (compare it to qApp->applicationVersion to check if an update is necessary)
     */
    void onlineVersionReceived(QString version);
    
    // ====================  STEP 2  ========================
    
public:
    
    /**
     * requires "getOnlineVersionInfo" to have succeeded
     * returns true if all files are identical to the online version
     */
    bool checkFiles();
    
    /**
     * requires "getOnlineVersionInfo" to have succeeded
     * returns the list of files that are different from the online version in this format : pair<filepath, size_in_bytes>
     */
    std::vector<std::pair<QString,qint64>> filesToUpdate();
    
    /**
     * requires "getOnlineVersionInfo" to have succeeded
     * returns true if a restart will be required for the update to complete (exe or dll files are in the update)
     */
    bool restartRequired();
    
    /**
     * progress getters, use this to get current file download progress when "progressChanged" signal is received
     * format is : pair<bytes downloaded, total bytes to download>
     */
    const std::unordered_map<QString, std::pair<qint64,qint64>>& getDetailedProgress();
    std::pair<qint64,qint64> getTotalProgress();
    
public slots:
    
    /**
     * requires "getOnlineVersionInfo" to have succeeded
     * downloads the files that are different from the online version
     * data files will go to the tmpData folder, exe files will go to tmpExe folder
     * listen to the "allFilesDownloaded" signal and the "progressChanged" signal to get the answer of thie request
     */
    void downloadFiles();
    
signals:
    
    /**
     * signal emitted when file downloading progress changed
     */
    void progressChanged();
    /**
     * signal emitted when all files are downloaded
     */
    void allFilesDownloaded();
    
    // ====================  STEP 3  ========================
    
public:
    
    /**
     * requires "downloadFiles" to have succeeded
     * copies the contents of tmpData to the program folder and deletes that folder
     * 
     * returns true on success
     */
    bool applyDataPatch();
    
    /**
     * requires "downloadFiles" to have succeeded
     * if the tmpExe folder exists, this will start a batch script that tries to copy files
     * from the tmpExe folder to the app folder every second until it succeeds, then starts the updated app,
     * it is your responsibility to quit the app as soon as you can so the update can complete.
     * 
     * returns true if the update script has been successfully launched
     * doesn't do anything and returns false if restartRequired() == false
     */
    bool applyExePatchAndRestart();
    
    // ====================  UTILS   ========================
    
public:
    
    /**
     * @brief parseAppFolder recursively parses the application dir
     * @param whitelist accepted file paths (these strings are QRegExp)
     * @param blacklist ignored  file paths (these strings are QRegExp)
     * @return a list of file paths, relative to the current application dir
     */
    static QStringList parseAppFolder(QStringList whitelist = {".*"}, QStringList blacklist = QStringList());
    
    /// serialization of version information
    static QByteArray generateVersionJson(QStringList dataFiles = parseAppFolder({".*"}, {".*\\.exe", ".*\\.dll"}),
                                          QStringList exeFiles  = parseAppFolder({".*\\.exe", ".*\\.dll"}));
    
    
private slots:
    void handleVersion(QByteArray versionJson);
    void handleFinished();
    
private:
    UpdaterClient* _client;
    int _currentStep;
    
    QStringList                            _remoteDataFiles;
    QByteArrayList                         _remoteDataHashes;
    QList<int>                             _remoteDataFileSizes;
    std::vector<std::pair<QString,qint64>> _missingDataFiles;
    QStringList                            _remoteExeFiles;
    QByteArrayList                         _remoteExeHashes;
    QList<int>                             _remoteExeFileSizes;
    std::vector<std::pair<QString,qint64>> _missingExeFiles;
};

#endif // VERSIONUPDATER_H
