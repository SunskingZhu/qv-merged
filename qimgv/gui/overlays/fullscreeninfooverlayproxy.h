#pragma once

#include "gui/overlays/fullscreeninfooverlay.h"

class FullscreenInfoOverlayProxy {
public:
    explicit FullscreenInfoOverlayProxy(FloatingWidgetContainer *parent = nullptr);
    ~FullscreenInfoOverlayProxy();
    void init();
    void show();
    void showWhenReady();
		void showWhenReady(int duration);
    void hide();
    void setInfo(const QStringList &info);

private:
		struct InfoOverlayStateBuffer {
				QStringList info;
				bool showImmediately = false;
				int duration = 0;
		};

    FloatingWidgetContainer *container;
    FullscreenInfoOverlay *infoOverlay;
    InfoOverlayStateBuffer stateBuf;
};
