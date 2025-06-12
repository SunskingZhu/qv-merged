#include "FileManager.h"
#include "Backup.h"

#include <QCryptographicHash>
#include <QDateTime>

bool QIV::FileManager::remove(const QFileInfo &target, bool trash)
{
	QString path = target.absoluteFilePath();

	if (target.exists() == false) {
		emit successReported(tr("Deletion not required, target does not exist\n%1").arg(path));
		return true;
	}

#ifdef Q_OS_WIN32
	if (target.isWritable() == false) {
		emit errorReported(tr("Deletion failed: source is not writable\n%1").arg(path));
		return false;
	}
#endif

	if (target.isDir()) {
		if (trash) {
			QFile file(path);
			if (file.moveToTrash()) {
				emit successReported(tr("Directory successfully moved to the trash!\n%1").arg(path));
				return true;
			}
		} else {
			QDir directory(path);
			if (directory.removeRecursively()) {
				emit successReported(tr("Directory successfully deleted!\n%1").arg(path));
				return true;
			}
		}

		emit errorReported(tr("Directory deletion failed: %1\n%2").arg(QFile().errorString(), path));
		return false;
	}

	QFile file(path);

	if (trash ? file.moveToTrash() : file.remove()) {
		emit successReported((trash
			? tr("File successfully moved to the trash!\n%1")
			: tr("File successfully deleted!\n%1"))
				.arg(path)
		);
		return true;
	}

	emit errorReported(tr("File deletion failed: %1\n%2").arg(file.errorString(), path));
	return false;
}

bool QIV::FileManager::remove(const QString &target, bool trash)
{
	return remove(QFileInfo(target), trash);
}

bool QIV::FileManager::perform(const QString &target, const QString &new_name, bool copy)
{
	return perform(QFileInfo(target), new_name, copy);
}

bool QIV::FileManager::perform(const QFileInfo &target, const QString &new_name, bool copy)
{
	QString operation = copy ? QStringLiteral("Copying") : QStringLiteral("Moving");

	if (new_name.isEmpty()) {
		emit errorReported(tr("%1 failed: empty destination name").arg(operation));
		return false;
	}

	const QFileInfo &source = target;
	QString source_path = source.absoluteFilePath();

	if (source.exists() == false) {
		emit errorReported(tr("%1 failed: source doesn't exists\n%2").arg(operation, source_path));
		return false;
	}

#ifdef Q_OS_WIN32
	if (source.isWritable() == false) {
		emit errorReported(tr("%1 failed: source is not writable\n%2").arg(operation, source_path));
		return false;
	}
#endif

	QFileInfo destination(source.absoluteDir().absoluteFilePath(new_name));
	QString destination_path = destination.absoluteFilePath();

	if (QDir().mkpath(destination.absolutePath()) == false) {
		emit errorReported(tr("%1 failed: cannot create a destination path\n%2").arg(operation, destination.absolutePath()));
		return false;
	}

	Backup *backup = nullptr;

	if (destination.exists()) {

#ifdef Q_OS_WIN32
		if (destination.isWritable() == false) {
			emit errorReported(tr("%1 failed: destination exists and is not writable\n%2").arg(operation, destination_path));
			return false;
		}
#endif

		backup = new Backup(destination);
		connect(backup, &Backup::errorOccurred, this, &FileManager::errorReported);

		if (backup->apply() == false) {
			delete backup;
			emit errorReported(tr("%1 failed: cannot create a backup\n%1").arg(operation, source_path));
			return false;
		}
	}

	QFile file(source.absoluteFilePath());

	if (copy ? file.copy(destination_path) : file.rename(destination_path)) {
		emit successReported((copy ? tr("Copied successfully!\n%2\n🡇\n%3") : tr("Moved successfully!\n%2\n🡇\n%3"))
			                   .arg(operation, source.absoluteFilePath(), destination_path)
		);
		delete backup;
		return true;
	}

	QString t1 = file.errorString();
	QString t2 = source.absoluteFilePath();

	emit errorReported(tr("%1 failed: %2\n%3\n🡇\n%4")
		                   .arg(operation, file.errorString(), source.absoluteFilePath(), destination_path)
	);

	if (backup) {
		backup->revert();
		delete backup;
	}

	return false;
}

bool QIV::FileManager::copy(const QString &target, const QString &new_name)
{
	return perform(target, new_name, true);
}

bool QIV::FileManager::copy(const QFileInfo &target, const QString &new_name)
{
	return perform(target, new_name, true);
}

bool QIV::FileManager::rename(const QString &target, const QString &new_name)
{
	return perform(target, new_name, false);
}

bool QIV::FileManager::rename(const QFileInfo &target, const QString &new_name)
{
	return perform(target, new_name, false);
}

bool QIV::FileManager::copyTo(const QString &target, const QString &new_path)
{
	QFileInfo source(target);
	QDir destination(new_path);
	return copy(source, destination.absoluteFilePath(source.fileName()));
}

bool QIV::FileManager::moveTo(const QString &target, const QString &new_path)
{
	QFileInfo source(target);
	QDir destination(new_path);
	return rename(source, destination.absoluteFilePath(source.fileName()));
}

QString QIV::FileManager::generateHash(const QString &target)
{
	return QCryptographicHash::hash(target.toUtf8(), QCryptographicHash::Md5).toHex();
}

bool QIV::FileManager::touch(const QString &target, const QDateTime &modification_time, const QDateTime &access_time)
{
	return this->touch(QFileInfo(target), modification_time, access_time);
}

bool QIV::FileManager::touch(const QFileInfo &target, const QDateTime &modification_time, const QDateTime &access_time)
{
	QFile file(target.absoluteFilePath());
	if (file.open(QIODevice::ReadWrite) == false) {
		emit errorReported(tr("Cannot touch file: %1\n%2").arg(file.errorString(), target.absoluteFilePath()));
		return false;
	}
	// file.setFileTime(srcBirthTime, QFileDevice::FileBirthTime); // TODO: does not work (linux)
	file.setFileTime(modification_time, QFileDevice::FileModificationTime);
	file.setFileTime(access_time, QFileDevice::FileAccessTime);
	file.close();
	return true;
}

QString QIV::FileManager::resolveFilePath(const QFileInfo &base, const QString &filename)
{
	return base.absoluteDir().absoluteFilePath(filename);
}