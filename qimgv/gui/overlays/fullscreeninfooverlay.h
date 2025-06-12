#pragma once

#include <QWidget>
#include <QTimer>
#include "gui/customwidgets/overlaywidget.h"

class FullscreenInfoOverlay : public OverlayWidget {
    Q_OBJECT

public:
    explicit FullscreenInfoOverlay(FloatingWidgetContainer *parent = nullptr);
    void setInfo(const QStringList &info);

		void show();
    void show(int duration);

private:
		QTimer visibilityTimer;
		int hideDelay;
};
