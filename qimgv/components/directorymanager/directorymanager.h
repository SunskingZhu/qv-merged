#pragma once

#include <QCollator>
#include <QElapsedTimer>
#include <QString>
#include <QSize>
#include <QDateTime>
#include <QRegularExpression>

#include <vector>
#include <string>
#include <iostream>
#include <filesystem>
#include <algorithm>

#include "settings.h"
#include "watchers/directorywatcher.h"
#include "utils/stuff.h"

#ifdef Q_OS_WIN32
#include "windows.h"
#endif

enum FileListSource { // rename? wip
    SOURCE_DIRECTORY,
    SOURCE_DIRECTORY_RECURSIVE,
    SOURCE_LIST
};

class DirectoryManager;

typedef bool (DirectoryManager::*CompareFunction)(const QFileInfo &info1, const QFileInfo &info2) const;

//TODO: rename? EntrySomething?

class DirectoryManager : public QObject {
    Q_OBJECT
public:
    DirectoryManager();

	bool setDirectory(const QString &path, bool recursive = false, bool watch = true);

	[[nodiscard]] int indexOfFile(const QFileInfo &info) const;
	[[nodiscard]] int indexOfDir(const QFileInfo &info) const;

	const QFileInfoList &files() const;

	void clear();

    QString directoryPath() const;
    int indexOfFile(QString filePath) const;
    int indexOfDir(QString dirPath) const;
    QString filePathAt(int index) const;
    unsigned long fileCount() const;
    unsigned long dirCount() const;
    bool isSupportedFile(QString filePath) const;
    bool isEmpty() const;

	[[nodiscard]] bool containsFile(const QFileInfo &info) const;
	[[nodiscard]] bool containsFile(const QString &path) const;
	[[nodiscard]] bool containsDir(const QFileInfo &info) const;
	[[nodiscard]] bool containsDir(const QString &path) const;

	inline bool isSupportedExtension(const QString &extension) const;

    QString fileNameAt(int index) const;
    QString prevOfFile(QString filePath) const;
    QString nextOfFile(QString filePath) const;
    QString prevOfDir(QString filePath) const;
    QString nextOfDir(QString filePath) const;
    void sortEntryLists();
    QDateTime lastModified(QString filePath) const;

    QString firstFile() const;
    QString lastFile() const;
    void setSortingMode(SortingMode mode);
    SortingMode sortingMode() const;

    bool isDir(QString path) const;

    unsigned long totalCount() const;

    const QFileInfo &fileEntryAt(int index) const;
    QString dirPathAt(int index) const;
    QString dirNameAt(int index) const;
    bool fileWatcherActive();

    bool insertFileEntry(const QString &filePath);
    bool forceInsertFileEntry(const QString &filePath);
    void removeFileEntry(const QString &filePath);
    void updateFileEntry(const QString &filePath);
    void renameFileEntry(const QString &oldFilePath, const QString &newName);

    bool insertDirEntry(const QString &dirPath);
    //bool forceInsertDirEntry(const QString &dirPath);
    void removeDirEntry(const QString &dirPath);
    //void updateDirEntry(const QString &dirPath);
    void renameDirEntry(const QString &oldDirPath, const QString &newName);

    FileListSource source() const;

	bool checkFileRange(int index) const;
	bool checkDirRange(int index) const;

private:
	QString m_directory_path;
	QCollator m_collator;
	QFileInfoList fileEntryVec, dirEntryVec;
    const QFileInfo defaultEntry;

	DirectoryWatcher *m_watcher = nullptr;

    void readSettings();
    SortingMode mSortingMode = SORT_NAME_ASC;
    FileListSource mListSource;

	bool m_directories_mode = false;
	QSet<QByteArray> m_supported_extensions;

public:
	[[nodiscard]] bool isDirectoriesMode() const;
	void setDirectoriesMode(bool directories_mode);

private:
	[[nodiscard]] QFileInfoList::const_iterator constFindFile(const QString &path) const;
	[[nodiscard]] QFileInfoList::const_iterator constFindDirectory(const QString &path) const;
	[[nodiscard]] QFileInfoList::const_iterator constFindFile(const QFileInfo &info) const;
	[[nodiscard]] QFileInfoList::const_iterator constFindDirectory(const QFileInfo &info) const;
	[[nodiscard]] QFileInfoList::iterator findFile(const QString &path);
	[[nodiscard]] QFileInfoList::iterator findDirectory(const QString &path);

	void loadEntryList(const QString &directory, bool recursive);
	void loadEntryList(bool recursive);

    bool path_entry_compare(const QFileInfo &e1, const QFileInfo &e2) const;
    bool path_entry_compare_reverse(const QFileInfo &e1, const QFileInfo &e2) const;
    bool name_entry_compare(const QFileInfo &e1, const QFileInfo &e2) const;
    bool name_entry_compare_reverse(const QFileInfo &e1, const QFileInfo &e2) const;
    bool date_entry_compare(const QFileInfo &e1, const QFileInfo &e2) const;
    bool date_entry_compare_reverse(const QFileInfo &e1, const QFileInfo &e2) const;
    CompareFunction compareFunction();
    bool size_entry_compare(const QFileInfo &e1, const QFileInfo &e2) const;
    bool size_entry_compare_reverse(const QFileInfo &e1, const QFileInfo &e2) const;
    void startFileWatcher(QString directoryPath);
    void stopFileWatcher();

	void addEntriesFromDirectory(const QString &path, bool recursive = false);

private slots:
    void onFileAddedExternal(QString fileName);
    void onFileRemovedExternal(QString fileName);
    void onFileModifiedExternal(QString fileName);
    void onFileRenamedExternal(QString oldFileName, QString newFileName);

signals:
	void errorOccurred(const QString &message);

    void loaded(const QString &path);
    void sortingChanged();
    void fileRemoved(QString filePath, int);
    void fileModified(QString filePath);
    void fileAdded(QString filePath);
    void fileRenamed(QString fromPath, int indexFrom, QString toPath, int indexTo);

    void dirRemoved(QString dirPath, int);
    void dirAdded(QString dirPath);
    void dirRenamed(QString fromPath, int indexFrom, QString toPath, int indexTo);
};
