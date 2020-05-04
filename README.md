# SparrowUpdater

Small library to handle auto-updating of Qt applications on Windows

## What it does :
- It has a permissive license
- Retrieves the latest version information of your app, the list of files, their sizes, and their checksums as json using HTTP
- compares the checksums to the local files
- download the missing files using HTTP
- replace the local files with the remote files, even if they require restarting the application
- the library has only 2 classes, it's easier to integrate it directly into your Qt app than linking it as a library 
- the library only uses Qt's network and core modules
- the API is simple and easily customizable, you have plenty of freedom over your updating process
- documented headers and simple code makes it easy to integrate, maintain and modify

## What it does not :
- deleting of local files, only adding new files and overriding existing ones is currently supported
- adding empty folders

## How to use it

### Bases

Grab the 2 classes VersionUpdater and UpdaterClient and add them to your project.
The interface of the library is the VersionUpdater class, its header is heavily documented though comments.
The BasicUpdater class is a Hello World for VersionUpdater, you can look at its code to get a rough idea of how to use the lib.

### test project

For a more advanced and complete example, you can look at the test project.

It is a GUI app allowing to test most features of the updater library, you can generate the version json for the update server by executing it with the command line option "makeVersion".
You can then copy every files in the bin folder to the testServer/htdocs folder.
Now you can start Miniweb (in the testServer folder), which is an extremely basic web server, it will emulate the remote server that provides updates.

You can now try to mess with the files in the bin folder and see how the application reacts by redownloading the missing or changed files.
