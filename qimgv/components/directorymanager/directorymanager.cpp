#include <QDirIterator>
#include "directorymanager.h"

namespace fs = std::filesystem;

DirectoryManager::DirectoryManager()
{
	m_collator.setNumericMode(true);

    readSettings();
    setSortingMode(settings->sortingMode());
    connect(settings, &Settings::settingsChanged, this, &DirectoryManager::readSettings);
}

template< typename T, typename Pred >
typename QList<T>::iterator
insert_sorted(QList<T> & vec, T const& item, Pred pred) {
    return vec.insert(std::upper_bound(vec.begin(), vec.end(), item, pred), item);
}

bool DirectoryManager::path_entry_compare(const QFileInfo &e1, const QFileInfo &e2) const {
    return m_collator.compare(e1.absoluteFilePath(), e2.absoluteFilePath()) < 0;
};

bool DirectoryManager::path_entry_compare_reverse(const QFileInfo &e1, const QFileInfo &e2) const {
    return m_collator.compare(e1.absoluteFilePath(), e2.absoluteFilePath()) > 0;
};

bool DirectoryManager::name_entry_compare(const QFileInfo &e1, const QFileInfo &e2) const {
    return m_collator.compare(e1.fileName(), e2.fileName()) < 0;
};

bool DirectoryManager::name_entry_compare_reverse(const QFileInfo &e1, const QFileInfo &e2) const {
    return m_collator.compare(e1.fileName(), e2.fileName()) > 0;
};

bool DirectoryManager::date_entry_compare(const QFileInfo& e1, const QFileInfo& e2) const {
    return e1.lastModified() < e2.lastModified();
}

bool DirectoryManager::date_entry_compare_reverse(const QFileInfo& e1, const QFileInfo& e2) const {
    return e1.lastModified() > e2.lastModified();
}

bool DirectoryManager::size_entry_compare(const QFileInfo& e1, const QFileInfo& e2) const {
    return e1.size() < e2.size();
}

bool DirectoryManager::size_entry_compare_reverse(const QFileInfo& e1, const QFileInfo& e2) const {
    return e1.size() > e2.size();
}

CompareFunction DirectoryManager::compareFunction()
{
	switch (mSortingMode) {
		case SORT_NAME_ASC:
			return &DirectoryManager::path_entry_compare;

		case SORT_NAME_DESC:
			return &DirectoryManager::path_entry_compare_reverse;

		case SORT_SIZE_ASC:
			return &DirectoryManager::size_entry_compare;

		case SORT_SIZE_DESC:
			return &DirectoryManager::size_entry_compare_reverse;

		case SORT_TIME_ASC:
			return &DirectoryManager::date_entry_compare;

		case SORT_TIME_DESC:
			return &DirectoryManager::date_entry_compare_reverse;
	}

	return &DirectoryManager::path_entry_compare;
}

void DirectoryManager::startFileWatcher(QString directoryPath) {
    if(directoryPath == "")
        return;

    if (m_watcher == nullptr) {
	    m_watcher = DirectoryWatcher::newInstance();
	    m_watcher->setParent(this);
		}

    connect(m_watcher, &DirectoryWatcher::fileCreated, this, &DirectoryManager::onFileAddedExternal, Qt::UniqueConnection);
    connect(m_watcher, &DirectoryWatcher::fileDeleted, this, &DirectoryManager::onFileRemovedExternal, Qt::UniqueConnection);
    connect(m_watcher, &DirectoryWatcher::fileModified, this, &DirectoryManager::onFileModifiedExternal, Qt::UniqueConnection);
    connect(m_watcher, &DirectoryWatcher::fileRenamed, this, &DirectoryManager::onFileRenamedExternal, Qt::UniqueConnection);

    m_watcher->setWatchPath(directoryPath);
    m_watcher->observe();
}

void DirectoryManager::stopFileWatcher() {
    if(!m_watcher)
        return;

    m_watcher->stopObserving();

    disconnect(m_watcher, &DirectoryWatcher::fileCreated, this, &DirectoryManager::onFileAddedExternal);
    disconnect(m_watcher, &DirectoryWatcher::fileDeleted, this, &DirectoryManager::onFileRemovedExternal);
    disconnect(m_watcher, &DirectoryWatcher::fileModified, this, &DirectoryManager::onFileModifiedExternal);
    disconnect(m_watcher, &DirectoryWatcher::fileRenamed, this, &DirectoryManager::onFileRenamedExternal);
}

// ##############################################################
// ####################### PUBLIC METHODS #######################
// ##############################################################

void DirectoryManager::readSettings()
{
	QList<QByteArray> supported_formats = settings->supportedFormats();
	m_supported_extensions = QSet<QByteArray>(supported_formats.constBegin(), supported_formats.constEnd());
}

bool DirectoryManager::setDirectory(const QString &path, bool recursive, bool watch)
{
	if (path.isEmpty()) {
		return false;
	}

	QDir directory(path);

	if (directory.isReadable() == false) {
		emit errorOccurred(QStringLiteral("Directory is not readable: %1").arg(directory.absolutePath()));
		return false;
	}

	if (recursive) {
		stopFileWatcher();
	}

	mListSource = recursive ? SOURCE_DIRECTORY_RECURSIVE : SOURCE_DIRECTORY;

	m_directory_path = directory.absolutePath();

	loadEntryList(recursive);

	emit loaded(m_directory_path);

	if (watch && recursive == false) {
		startFileWatcher(m_directory_path);
	}

	return true;
}

QString DirectoryManager::directoryPath() const
{
	return m_directory_path;
}

int DirectoryManager::indexOfFile(const QFileInfo &info) const
{
	return fileEntryVec.indexOf(info);
}

int DirectoryManager::indexOfDir(const QFileInfo &info) const
{
	return dirEntryVec.indexOf(info);
}

int DirectoryManager::indexOfFile(QString filePath) const
{
	return indexOfFile(QFileInfo(filePath));
}

int DirectoryManager::indexOfDir(QString dirPath) const
{
	return indexOfDir(QFileInfo(dirPath));
}

const QFileInfoList &DirectoryManager::files() const
{
	return fileEntryVec;
}

QString DirectoryManager::filePathAt(int index) const {
    return checkFileRange(index) ? fileEntryVec.at(index).absoluteFilePath() : "";
}

QString DirectoryManager::fileNameAt(int index) const {
    return checkFileRange(index) ? fileEntryVec.at(index).fileName() : "";
}

QString DirectoryManager::dirPathAt(int index) const {
    return checkDirRange(index) ? dirEntryVec.at(index).absoluteFilePath() : "";
}

QString DirectoryManager::dirNameAt(int index) const {
    return checkDirRange(index) ? dirEntryVec.at(index).fileName() : "";
}

QString DirectoryManager::firstFile() const {
    QString filePath = "";
    if(fileEntryVec.size())
        filePath = fileEntryVec.front().absoluteFilePath();
    return filePath;
}

QString DirectoryManager::lastFile() const {
    QString filePath = "";
    if(fileEntryVec.size())
        filePath = fileEntryVec.back().absoluteFilePath();
    return filePath;
}

QString DirectoryManager::prevOfFile(QString filePath) const {
    QString prevFilePath = "";
    int currentIndex = indexOfFile(filePath);
    if(currentIndex > 0)
        prevFilePath = fileEntryVec.at(currentIndex - 1).absoluteFilePath();
    return prevFilePath;
}

QString DirectoryManager::nextOfFile(QString filePath) const {
    QString nextFilePath = "";
    int currentIndex = indexOfFile(filePath);
    if(currentIndex >= 0 && currentIndex < fileEntryVec.size() - 1)
        nextFilePath = fileEntryVec.at(currentIndex + 1).absoluteFilePath();
    return nextFilePath;
}

QString DirectoryManager::prevOfDir(QString dirPath) const {
    QString prevDirectoryPath = "";
    int currentIndex = indexOfDir(dirPath);
    if(currentIndex > 0)
        prevDirectoryPath = dirEntryVec.at(currentIndex - 1).absoluteFilePath();
    return prevDirectoryPath;
}

QString DirectoryManager::nextOfDir(QString dirPath) const {
    QString nextDirectoryPath = "";
    int currentIndex = indexOfDir(dirPath);
    if(currentIndex >= 0 && currentIndex < dirEntryVec.size() - 1)
        nextDirectoryPath = dirEntryVec.at(currentIndex + 1).absoluteFilePath();
    return nextDirectoryPath;
}

bool DirectoryManager::checkFileRange(int index) const {
    return index >= 0 && index < (int)fileEntryVec.size();
}

bool DirectoryManager::checkDirRange(int index) const {
    return index >= 0 && index < (int)dirEntryVec.size();
}

unsigned long DirectoryManager::totalCount() const {
    return fileCount() + dirCount();
}

unsigned long DirectoryManager::fileCount() const {
    return fileEntryVec.size();
}

unsigned long DirectoryManager::dirCount() const {
    return dirEntryVec.size();
}

const QFileInfo &DirectoryManager::fileEntryAt(int index) const {
    if(checkFileRange(index))
        return fileEntryVec.at(index);
    else
        return defaultEntry;
}

QDateTime DirectoryManager::lastModified(QString filePath) const {
    QFileInfo info;
    if(containsFile(filePath))
        info.setFile(filePath);
    return info.lastModified();
}

bool DirectoryManager::isSupportedFile(QString path) const
{
	QFileInfo info(path);
	while (info.isSymLink()) {
		info = QFileInfo(info.symLinkTarget());
	}
	return info.isFile() && this->isSupportedExtension(info.suffix());
}

bool DirectoryManager::isDir(QString path) const {
    if(!std::filesystem::exists(toStdString(path)))
        return false;
    if(!std::filesystem::is_directory(toStdString(path)))
        return false;
    return true;
}

bool DirectoryManager::isEmpty() const {
    return fileEntryVec.empty();
}


bool DirectoryManager::isSupportedExtension(const QString &extension) const
{
	return m_supported_extensions.contains(extension.toLower().toLatin1());
}

bool DirectoryManager::containsFile(const QFileInfo &info) const
{
	return constFindFile(info) != fileEntryVec.end();
}

bool DirectoryManager::containsFile(const QString &path) const
{
	return constFindFile(path) != fileEntryVec.end();
}

bool DirectoryManager::containsDir(const QFileInfo &info) const
{
	return constFindDirectory(info) != dirEntryVec.end();
}

bool DirectoryManager::containsDir(const QString &path) const
{
	return constFindDirectory(path) != dirEntryVec.end();
}

// ##############################################################
// ###################### PRIVATE METHODS #######################
// ##############################################################

void DirectoryManager::loadEntryList(const QString &directory, bool recursive)
{
	dirEntryVec.clear();
	fileEntryVec.clear();
	addEntriesFromDirectory(directory, recursive);
}

void DirectoryManager::loadEntryList(bool recursive)
{
	loadEntryList(m_directory_path, recursive);
}

void DirectoryManager::addEntriesFromDirectory(const QString &path, bool recursive)
{
	QDir::Filters filters = QDir::NoDotAndDotDot;

	if (this->isDirectoriesMode()) {
		filters |= QDir::AllDirs;
	} else {
		filters |= QDir::Files;
	}

	if (settings->showHiddenFiles()) {
		filters |= QDir::Hidden;
	}

	QDirIterator iterator(path, filters, recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);
	QFileInfoList list;

	list.reserve(100);

	if (this->isDirectoriesMode()) {
		while (iterator.hasNext()) {
			list << iterator.nextFileInfo();
		}
	} else {
		while (iterator.hasNext()) {
			QFileInfo info = iterator.nextFileInfo();
			if (this->isSupportedExtension(info.suffix())) {
				list << info;
			}
		}
	}

	if (list.isEmpty()) {
		return;
	}

	std::sort(list.begin(), list.end(), std::bind(compareFunction(), this, std::placeholders::_1, std::placeholders::_2));

	QFileInfoList &entries = this->isDirectoriesMode() ? dirEntryVec : fileEntryVec;

	entries.reserve(entries.size() + list.size());

	for (const QFileInfo &info : list) {
		entries.emplace_back(info);
	}
}

void DirectoryManager::sortEntryLists() {
    if(settings->sortFolders())
        std::sort(dirEntryVec.begin(), dirEntryVec.end(), std::bind(compareFunction(), this, std::placeholders::_1, std::placeholders::_2));
    else
        std::sort(dirEntryVec.begin(), dirEntryVec.end(), std::bind(&DirectoryManager::path_entry_compare, this, std::placeholders::_1, std::placeholders::_2));
    std::sort(fileEntryVec.begin(), fileEntryVec.end(), std::bind(compareFunction(), this, std::placeholders::_1, std::placeholders::_2));
}

void DirectoryManager::setSortingMode(SortingMode mode) {
    if(mode != mSortingMode) {
        mSortingMode = mode;
        if(fileEntryVec.size() > 1 || dirEntryVec.size() > 1) {
            sortEntryLists();
            emit sortingChanged();
        }
    }
}

SortingMode DirectoryManager::sortingMode() const {
    return mSortingMode;
}

// Entry management

bool DirectoryManager::insertFileEntry(const QString &filePath) {
    if(!isSupportedFile(filePath))
        return false;
    return forceInsertFileEntry(filePath);
}

// skips filename regex check
bool DirectoryManager::forceInsertFileEntry(const QString &filePath)
{
	QFileInfo info(filePath);
	if (info.isFile() == false || this->containsFile(info)) {
		return false;
	}
  insert_sorted(fileEntryVec, info, std::bind(compareFunction(), this, std::placeholders::_1, std::placeholders::_2));
  if(!directoryPath().isEmpty()) {
      qDebug() << "fileIns" << filePath << directoryPath();
      emit fileAdded(filePath);
  }
  return true;
}

void DirectoryManager::removeFileEntry(const QString &filePath)
{
	QFileInfoList::const_iterator info = constFindFile(filePath);
	if (info == fileEntryVec.constEnd()) {
		return;
	}

	uint index = std::distance(fileEntryVec.constBegin(), info);

	fileEntryVec.erase(info);

  emit fileRemoved(filePath, index);
}

void DirectoryManager::updateFileEntry(const QString &filePath)
{
	QFileInfoList::iterator info = findFile(filePath);
	if (info == fileEntryVec.end()) {
		return;
	}

	info->refresh();

	emit fileModified(filePath);
}

void DirectoryManager::renameFileEntry(const QString &oldFilePath, const QString &newFileName) {
    QFileInfo fi(oldFilePath);
    QString newFilePath = fi.absolutePath() + "/" + newFileName;
    if(!containsFile(oldFilePath)) {
        if(containsFile(newFilePath))
            updateFileEntry(newFilePath);
        else
            insertFileEntry(newFilePath);
        return;
    }
    if(!isSupportedFile(newFilePath)) {
        removeFileEntry(oldFilePath);
        return;
    }
    if(containsFile(newFilePath)) {
        int replaceIndex = indexOfFile(newFilePath);
        fileEntryVec.erase(fileEntryVec.begin() + replaceIndex);
        emit fileRemoved(newFilePath, replaceIndex);
    }
    // remove the old one
    int oldIndex = indexOfFile(oldFilePath);
    fileEntryVec.erase(fileEntryVec.begin() + oldIndex);
    // insert
    insert_sorted(fileEntryVec, QFileInfo(newFilePath), std::bind(compareFunction(), this, std::placeholders::_1, std::placeholders::_2));
    qDebug() << "fileRen" << oldFilePath << newFilePath;
    emit fileRenamed(oldFilePath, oldIndex, newFilePath, indexOfFile(newFilePath));
}

// ---- dir entries

bool DirectoryManager::insertDirEntry(const QString &dirPath) {
    if(containsDir(dirPath))
        return false;

    insert_sorted(dirEntryVec, QFileInfo(dirPath), std::bind(compareFunction(), this, std::placeholders::_1, std::placeholders::_2));
    emit dirAdded(dirPath);
    return true;
}

void DirectoryManager::removeDirEntry(const QString &dirPath) {
    if(!containsDir(dirPath))
        return;
    int index = indexOfDir(dirPath);
    dirEntryVec.erase(dirEntryVec.begin() + index);
    qDebug() << "dirRem" << dirPath;
    emit dirRemoved(dirPath, index);
}

void DirectoryManager::renameDirEntry(const QString &oldDirPath, const QString &newDirName) {
    if(!containsDir(oldDirPath))
        return;
    QFileInfo fi(oldDirPath);
    QString newDirPath = fi.absolutePath() + "/" + newDirName;
    // remove the old one
    int oldIndex = indexOfDir(oldDirPath);
    dirEntryVec.erase(dirEntryVec.begin() + oldIndex);
    // insert
	insert_sorted(dirEntryVec, QFileInfo(newDirPath), std::bind(compareFunction(), this, std::placeholders::_1, std::placeholders::_2));
  emit dirRenamed(oldDirPath, oldIndex, newDirPath, indexOfDir(newDirPath));
}


FileListSource DirectoryManager::source() const {
    return mListSource;
}

bool DirectoryManager::fileWatcherActive()
{
	return m_watcher && m_watcher->isObserving();
}

//----------------------------------------------------------------------------
// fs m_watcher events  ( onFile___External() )
// these take file NAMES, not paths
void DirectoryManager::onFileRemovedExternal(QString fileName) {
    QString fullPath = m_watcher->watchPath() + "/" + fileName;
    removeDirEntry(fullPath);
    removeFileEntry(fullPath);
}

void DirectoryManager::onFileAddedExternal(QString fileName) {
    QString fullPath = m_watcher->watchPath() + "/" + fileName;
    if(isDir(fullPath))
        insertDirEntry(fullPath);
    else
        insertFileEntry(fullPath);
}

void DirectoryManager::onFileRenamedExternal(QString oldName, QString newName) {
    QString oldPath = m_watcher->watchPath() + "/" + oldName;
    QString newPath = m_watcher->watchPath() + "/" + newName;
    if(isDir(newPath))
        renameDirEntry(oldPath, newName);
    else
        renameFileEntry(oldPath, newName);
}

void DirectoryManager::onFileModifiedExternal(QString fileName) {
    updateFileEntry(m_watcher->watchPath() + "/" + fileName);
}

bool DirectoryManager::isDirectoriesMode() const
{
	return m_directories_mode;
}

void DirectoryManager::setDirectoriesMode(bool directories_mode)
{
	m_directories_mode = directories_mode;
}

QFileInfoList::const_iterator DirectoryManager::constFindFile(const QString &path) const
{
	return std::find_if(fileEntryVec.constBegin(), fileEntryVec.constEnd(), [&] (const QFileInfo &info){
		return info.absoluteFilePath() == path;
	});
}

QFileInfoList::const_iterator DirectoryManager::constFindDirectory(const QString &path) const
{
	return std::find_if(dirEntryVec.constBegin(), dirEntryVec.constEnd(), [&] (const QFileInfo &info){
		return info.absoluteFilePath() == path;
	});
}

QFileInfoList::const_iterator DirectoryManager::constFindFile(const QFileInfo &info) const
{
	return std::find(fileEntryVec.begin(), fileEntryVec.end(), info);
}

QFileInfoList::const_iterator DirectoryManager::constFindDirectory(const QFileInfo &info) const
{
	return std::find(dirEntryVec.begin(), dirEntryVec.end(), info);
}

QFileInfoList::iterator DirectoryManager::findFile(const QString &path)
{
	return std::find_if(fileEntryVec.begin(), fileEntryVec.end(), [&] (const QFileInfo &info){
		return info.absoluteFilePath() == path;
	});
}

QFileInfoList::iterator DirectoryManager::findDirectory(const QString &path)
{
	return std::find_if(dirEntryVec.begin(), dirEntryVec.end(), [&] (const QFileInfo &info){
		return info.absoluteFilePath() == path;
	});
}

void DirectoryManager::clear()
{
	dirEntryVec.clear();
	fileEntryVec.clear();
	m_directory_path.clear();
}
