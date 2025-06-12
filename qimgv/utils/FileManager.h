#ifndef QIMGV_FILEMANAGER_H
#define QIMGV_FILEMANAGER_H

#include <QFileInfo>
#include <QDir>

#ifdef Q_OS_WIN32
#include "windows.h"
#endif

namespace QIV
{
	class FileManager : public QObject
	{
	Q_OBJECT

	public:
		using QObject::QObject;

		bool touch(const QString &target, const QDateTime &modification_time, const QDateTime &access_time);
		bool touch(const QFileInfo &target, const QDateTime &modification_time, const QDateTime &access_time);

		static QString generateHash(const QString &target);

		bool copy(const QString &target, const QString &new_name);
		bool copy(const QFileInfo &target, const QString &new_name);
		bool rename(const QString &target, const QString &new_name);
		bool rename(const QFileInfo &target, const QString &new_name);

		bool remove(const QString &target, bool trash = false);
		bool remove(const QFileInfo &target, bool trash = false);

		bool copyTo(const QString &target, const QString &new_path);
		bool moveTo(const QString &target, const QString &new_path);

		static QString resolveFilePath(const QFileInfo &base, const QString &filename);

	signals:
		void errorReported(const QString &message);
		void warningReported(const QString &message);
		void successReported(const QString &message);

	protected:
		bool perform(const QString &target, const QString &new_name, bool copy);
		bool perform(const QFileInfo &target, const QString &new_name, bool copy);
	};
} // QIV

#endif //QIMGV_FILEMANAGER_H
