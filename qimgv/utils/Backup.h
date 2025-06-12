#ifndef QIMGV_BACKUP_H
#define QIMGV_BACKUP_H

#include <QFileInfo>

namespace QIV
{
	class Backup : public QObject
	{
	Q_OBJECT
	public:
		explicit Backup(const QFileInfo &target);

		[[nodiscard]] QString resolveBackupName() const;
		[[nodiscard]] bool isApplied() const;
		bool apply();
		bool revert();

		void attach(const QString &backup_name);
		void detach();

	signals:
		void errorOccurred(const QString &message);

	private:
		QFileInfo m_target;
		QString m_backup_name;
	};
} // QIV

#endif //QIMGV_BACKUP_H
