#include "cmdoptionsrunner.h"

void CmdOptionsRunner::generateThumbs(QString dirPath, int size) {
    if(size <= 50 || size > 400) {
        qDebug() << "Error: Invalid thumbnail size.";
        qDebug() << "Please specify a value between [50, 400].";
        qDebug() << "Example:  qimgv --gen-thumbs=/home/user/Pictures/ --gen-thumbs-size=120";
        QCoreApplication::exit(1);
        return;
    }

    Thumbnailer th;
    DirectoryManager dm;
    if(!dm.setDirectory(dirPath, true, false)) {
        qDebug() << "Error: Invalid path.";
        QCoreApplication::exit(1);
        return;
    }

    const QFileInfoList &files = dm.files();

    qDebug() << "\nDirectory:" << dirPath;
    qDebug() << "File count:" << files.size();
    qDebug() << "Size limit:" << size << "x" << size << "px";
    qDebug() << "Generating thumbnails...";

    for (const QFileInfo &file : files) {
	    th.getThumbnailAsync(file.absoluteFilePath(), size, false, false);
		}

    th.waitForDone();
    qDebug() << "\nDone.";
    QCoreApplication::quit();
}

void CmdOptionsRunner::showBuildOptions() {
    QStringList features;
#ifdef USE_MPV
    features << "USE_MPV";
#endif
#ifdef USE_EXIV2
    features << "USE_EXIV2";
#endif
#ifdef USE_KDE_BLUR
    features << "USE_KDE_BLUR";
#endif
#ifdef USE_OPENCV
    features << "USE_OPENCV";
#endif
    qDebug() << "\nEnabled build options:";
    if(!features.count())
        qDebug() << "   --";
    for(int i = 0; i < features.count(); i++)
        qDebug() << "   " << features.at(i);
    QCoreApplication::quit();
}
