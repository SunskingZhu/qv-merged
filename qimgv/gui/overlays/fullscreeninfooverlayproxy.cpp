#include "fullscreeninfooverlayproxy.h"

FullscreenInfoOverlayProxy::FullscreenInfoOverlayProxy(FloatingWidgetContainer *parent)
    : container(parent),
      infoOverlay(nullptr)
{
}

FullscreenInfoOverlayProxy::~FullscreenInfoOverlayProxy() {
    if(infoOverlay)
        infoOverlay->deleteLater();
}

void FullscreenInfoOverlayProxy::show() {
    stateBuf.showImmediately = true;
    init();
}

void FullscreenInfoOverlayProxy::showWhenReady() {
    if(!infoOverlay)
        stateBuf.showImmediately = true;
    else
			infoOverlay->show();
}

void FullscreenInfoOverlayProxy::showWhenReady(int duration)
{
	if(!infoOverlay) {
			stateBuf.showImmediately = true;
			stateBuf.duration = duration;
	} else {
		infoOverlay->show(duration);
	}
}

void FullscreenInfoOverlayProxy::hide() {
    stateBuf.showImmediately = false;
    if(infoOverlay)
        infoOverlay->hide();
}

void FullscreenInfoOverlayProxy::setInfo(const QStringList &_info) {
    if(infoOverlay) {
        infoOverlay->setInfo(_info);
    } else {
        stateBuf.info = _info;
    }
}

void FullscreenInfoOverlayProxy::init() {
    if(infoOverlay)
        return;
    infoOverlay = new FullscreenInfoOverlay(container);
    if(!stateBuf.info.isEmpty())
        setInfo(stateBuf.info);
		if(stateBuf.showImmediately) {
			if (stateBuf.duration) {
				infoOverlay->show(stateBuf.duration);
			} else {
				infoOverlay->show();
			}
		}
}
