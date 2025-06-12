#include "fullscreeninfooverlay.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QFont>

FullscreenInfoOverlay::FullscreenInfoOverlay(FloatingWidgetContainer *parent) : OverlayWidget(parent)
{
    setPosition(FloatingWidgetPosition::TOPLEFT);
    this->setHorizontalMargin(0);
    this->setVerticalMargin(0);

		this->setLayout(new QVBoxLayout());
		this->setInfo(QStringList() << "No file opened");

    if(parent)
        setContainerSize(parent->size());

		hideDelay = 2000;
    visibilityTimer.setSingleShot(true);
    visibilityTimer.setInterval(hideDelay);

		connect(&visibilityTimer, &QTimer::timeout, this, &FullscreenInfoOverlay::hideAnimated);
}

void FullscreenInfoOverlay::setInfo(const QStringList &info)
{
	this->blockSignals(true);

	QLayout *layout = this->layout();

	while (layout->count() > info.count()) {
		QLayoutItem *item;
		if ((item = layout->takeAt(0)) != nullptr) {
			delete item->widget();
			delete item;
		}
	}

	while (layout->count() < info.count()) {
		QLabel *label = new QLabel(this);
		QFont font = label->font();
		font.setPointSize(12);
		label->setFont(font);
		layout->addWidget(label);
	}

	int index = 0;
	foreach (const QString &text, info) {
		QLayoutItem *item =	layout->itemAt(index++);
		static_cast<QLabel *>(item->widget())->setText(text);
	}

	this->blockSignals(false);

	this->adjustSize();
}

void FullscreenInfoOverlay::show()
{
	OverlayWidget::show();
}

// "blink" the widget; show then fade out immediately
void FullscreenInfoOverlay::show(int duration)
{
	visibilityTimer.stop();
	OverlayWidget::show();
	// fade out after delay
	visibilityTimer.setInterval(duration);
	visibilityTimer.start();
}