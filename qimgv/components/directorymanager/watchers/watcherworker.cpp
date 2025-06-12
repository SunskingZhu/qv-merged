#include "watcherworker.h"
#include <QDebug>

WatcherWorker::WatcherWorker()
{
}

void WatcherWorker::setRunning(bool running)
{
	bool previous = isRunning.fetchAndStoreRelaxed(running);
	if (previous != running && previous) {
		stopped();
	}
}

void WatcherWorker::stopped()
{}
