#ifndef BASICUPDATER_H
#define BASICUPDATER_H

#include <QObject>

class VersionUpdater;

/**
 * @brief The BasicUpdater class is the most basic but complete updater class you can use in your project
 * 
 * You can see it as a Hello World for this library
 * 
 * Look at the test project for a more complete example
 */
class BasicUpdater : public QObject
{
    Q_OBJECT
public:
    explicit BasicUpdater(QObject *parent = nullptr, QString baseUrl = "http://localhost/");
    
public slots:
    virtual void updateApplication();
    
signals:
    void failure(QStringList errors);
    void progressChanged(qint64 progress, qint64 total);
    void success();

protected:
    VersionUpdater* _updater;
};

#endif // BASICUPDATER_H
