#include "FloatingMessage.h"
#include "ui_FloatingMessage.h"

#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

QIV::FloatingMessage::FloatingMessage(QWidget *parent) : QWidget(parent), ui(new Ui::FloatingMessage)
{
	ui->setupUi(this);

	this->setAccessibleName("FloatingMessage");

	this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	ui->textLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

	auto opacity_effect = new QGraphicsOpacityEffect(this);
	opacity_effect->setOpacity(1.0);
	this->setGraphicsEffect(opacity_effect);

	m_fade_animation = new QPropertyAnimation(opacity_effect, "opacity", this);
	m_fade_animation->setDuration(500);
	m_fade_animation->setStartValue(1.0f);
	m_fade_animation->setEndValue(0.0f);
	m_fade_animation->setEasingCurve(QEasingCurve::OutQuad);
	connect(m_fade_animation, &QPropertyAnimation::finished, this, &QWidget::deleteLater);
}

QIV::FloatingMessage::~FloatingMessage()
{
	delete ui;
}

void QIV::FloatingMessage::setIcon(QIV::FloatingMessageIcon icon)
{
	switch (icon) {
		case FloatingMessageIcon::NO_ICON:
		case FloatingMessageIcon::ICON_WARNING:
		case FloatingMessageIcon::ICON_ERROR:
			//ui->iconLabel->setIconPath(":/res/icons/common/notifications/error16.png");
			ui->iconLabel->hide();
			break;

		case FloatingMessageIcon::ICON_DIRECTORY:
			ui->iconLabel->show();
			ui->iconLabel->setIconPath(":/res/icons/common/buttons/panel/folder16.png");
			break;

		case FloatingMessageIcon::ICON_LEFT_EDGE:
			ui->iconLabel->show();
			ui->iconLabel->setIconPath(":/res/icons/common/notifications/dir_start20.png");
			break;

		case FloatingMessageIcon::ICON_RIGHT_EDGE:
			ui->iconLabel->show();
			ui->iconLabel->setIconPath(":/res/icons/common/notifications/dir_end20.png");
			break;

		case FloatingMessageIcon::ICON_SUCCESS:
			ui->iconLabel->show();
			ui->iconLabel->setIconPath(":/res/icons/common/notifications/success16.png");
			break;
	}

	this->adjustSize();
}

void QIV::FloatingMessage::setText(const QString &text)
{
	ui->textLabel->setText(text);
	ui->textLabel->setVisible(text.isEmpty() == false);
	this->adjustSize();
}

void QIV::FloatingMessage::paintEvent(QPaintEvent *event)
{
	QStyleOption option;
	option.initFrom(this);
	QPainter painter(this);
	style()->drawPrimitive(QStyle::PE_Widget, &option, &painter, this);
}

void QIV::FloatingMessage::discard()
{
	m_fade_animation->stop();
	m_fade_animation->start(QPropertyAnimation::KeepWhenStopped);
}
