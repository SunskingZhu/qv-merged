#include "scalerrunnable.h"

#include <QElapsedTimer>

ScalerRunnable::ScalerRunnable() {
}

void ScalerRunnable::setRequest(ScalerRequest r) {
    req = r;
}

void ScalerRunnable::run() {
    emit started(req);
    //QElapsedTimer t;
    //t.start();
    QImage *scaled = ImageLib::scaled(req.image->getImage(), req.size, req.filter ? req.filter : QI_FILTER_NEAREST);
		//qDebug() << ">> " << (int) req.filter << " " << req.size << ": " << t.elapsed();
    emit finished(scaled, req);
}
