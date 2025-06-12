#pragma once

#include <QWidget>

class QPropertyAnimation;

namespace QIV
{
	namespace Ui {
		class FloatingMessage;
	}

	enum FloatingMessageIcon {
		NO_ICON,
		ICON_DIRECTORY,
		ICON_LEFT_EDGE,
		ICON_RIGHT_EDGE,
		ICON_SUCCESS,
		ICON_WARNING,
		ICON_ERROR
	};

	class FloatingMessage : public QWidget
	{
		Q_OBJECT
	public:
		explicit FloatingMessage(QWidget *parent = nullptr);
		~FloatingMessage() override;

	public slots:
		void setIcon(FloatingMessageIcon icon);
		void setText(const QString &text);
		void discard();

	protected:
		void paintEvent(QPaintEvent *event) override;

	private:
		Ui::FloatingMessage *ui;
		QPropertyAnimation *m_fade_animation = nullptr;
	};
} // QIV
