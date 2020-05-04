#include <QApplication>
#include <QFile>

#include "SparrowUpdater/versionupdater.h"

#include "mainwindow.h"

#ifndef GIT_CURRENT
#define GIT_CURRENT "unknown"
#endif

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_REVISION GIT_CURRENT

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationVersion(QString("%1.%2.%3").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_REVISION));
    
    if(a.arguments().contains("makeVersion"))
    {
        // generate json
        QStringList exeFiles = VersionUpdater::parseAppFolder({"test.exe", ".*\\.dll"});
        QStringList dataFiles = VersionUpdater::parseAppFolder({"data.*"});
        QByteArray versionJson = VersionUpdater::generateVersionJson(dataFiles, exeFiles);
        
        // save json
        QFile file("version.json");
        file.open(QFile::WriteOnly);
        file.write(versionJson);
        return 0;
    }
    
    MainWindow mainwindow;
    mainwindow.show();
    return a.exec();
}
