#include "Backup.h"

QIV::Backup::Backup(const QFileInfo &target) : m_target(target)
{

}

QString QIV::Backup::resolveBackupName() const
{
	QString result;
	QString base_new_name = m_target.absolutePath() + '/' + m_target.completeBaseName() + '.';
	int index = 0;

	do {
		result = base_new_name + QString::number(++index) + '.' + m_target.suffix();
	} while (QFileInfo::exists(result));

	return result;
}

bool QIV::Backup::isApplied() const
{
	return m_backup_name.isEmpty() == false;
}

bool QIV::Backup::apply()
{
	if (this->isApplied() == false) {
		m_backup_name = this->resolveBackupName();
		if (QFile::rename(m_target.absoluteFilePath(), m_backup_name) == false) {

			emit errorOccurred(tr("Cannot back up an already existing file, renaming failed:\n%1\n->\n%2")
				.arg(m_target.absoluteFilePath())
				.arg(m_backup_name)
			);

			m_backup_name.clear();
			return false;
		}
	}
	return true;
}

bool QIV::Backup::revert()
{
	if (this->isApplied() == false) {
		return false;
	}

	if (m_target.exists()) {
		emit errorOccurred(tr("Cannot revert the backup, the file already exists:\n%1\n->\n%2")
			.arg(m_backup_name)
			.arg(m_target.absoluteFilePath())
		);
		return false;
	}

	if (QFile::rename(m_backup_name, m_target.absoluteFilePath()) == false) {
		emit errorOccurred(tr("Cannot revert the backup, renaming failed:\n%1\n->\n%2")
			.arg(m_backup_name)
			.arg(m_target.absoluteFilePath())
		);
		return false;
	}

	this->detach();
	return true;
}

void QIV::Backup::detach()
{
	m_backup_name.clear();
}
