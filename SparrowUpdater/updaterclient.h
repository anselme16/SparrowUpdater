#ifndef UPDATERCLIENT_H
#define UPDATERCLIENT_H

#include <QObject>
#include <memory>

class QNetworkAccessManager;
class Version;
class QNetworkReply;

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

class UpdaterClient : public QObject
{
	Q_OBJECT
public:
	explicit UpdaterClient(QObject* parent, const QString& baseUrl);
    virtual ~UpdaterClient() {}
    
    /// request data from the server
    void getLastVersion();
    void getFile(QString filename, QString dstDir);
    
    /// get error information
    bool hasFailed() { return _hasFailed; }
    QStringList errors() { return _errors; }
    
    /// get progress information
    const std::unordered_map<QString, std::pair<qint64,qint64>>& getDetailedProgress() { return _progress; }
    std::pair<qint64,qint64> getTotalProgress();
    
signals:
    /// answers to requests
    void failed();
    void receivedLastVersion(QByteArray versionJson);
    void allFilesReceived();
    
    /// use getDetailedProgress, or getTotalProgress to get the new progress values
    void progressChanged();

private slots:
    void handleVersion(QNetworkReply* reply);
    void handleFile(QNetworkReply *reply, QString filename, QString dstDir);
    
private:
    std::unordered_map<QString, std::pair<qint64,qint64>> _progress;    
	QNetworkAccessManager* manager;
    size_t nbFilesPending;
    QString _baseUrl;
    bool _hasFailed;
    QStringList _errors;
};

#endif // UPDATERCLIENT_H
