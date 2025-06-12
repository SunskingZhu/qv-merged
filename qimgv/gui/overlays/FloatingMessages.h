#pragma once

#include "gui/customwidgets/floatingwidget.h"
#include "gui/widgets/FloatingMessage.h"

namespace QIV
{
	class FloatingMessages : public FloatingWidget
	{
	Q_OBJECT
	public:
		static constexpr int DURATION_FOREVER = -1;
		static constexpr int DURATION_DEFAULT = 1700;
		static constexpr int DURATION_SUCCESS = 1500;
		static constexpr int DURATION_WARNING = 1500;
		static constexpr int DURATION_ERROR = 2800;

		FloatingMessages(FloatingWidgetContainer *parent);

		FloatingMessage *addMessage(const QString &text, FloatingMessageIcon icon = NO_ICON, int duration = DURATION_DEFAULT);

		const QMargins &margins() const;
		void setMargins(const QMargins &margins);

		const Qt::Alignment &alignment() const;
		void setAlignment(const Qt::Alignment &alignment);

	public slots:
		void error(const QString &text);
		void warning(const QString &text);
		void success(const QString &text);

	protected:
		void mousePressEvent(QMouseEvent *event);
		virtual void resizeEvent(QResizeEvent *event);

		virtual void recalculateGeometry();

	private slots:
		void readSettings();

	private:
		QMargins m_margins;
		Qt::Alignment m_alignment = Qt::AlignBottom | Qt::AlignLeft;
	};
}
