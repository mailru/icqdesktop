#include "stdafx.h"
#include "MessageAlertWidget.h"
#include "../contact_list/RecentItemDelegate.h"
#include "../../cache/avatars/AvatarStorage.h"

namespace Ui
{
	MessageAlertWidget::MessageAlertWidget(const Data::DlgState& state, Logic::RecentItemDelegate* delegate, QWidget* parent)
		: QWidget(parent)
		, Delegate_(delegate)
		, State_(state)
		, Painter_(0)
	{
		Options_.initFrom(this);
		setFixedSize(Delegate_->sizeHintForAlert());
        connect(Logic::GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(avatarChanged(QString)), Qt::QueuedConnection);
	}

	MessageAlertWidget::~MessageAlertWidget()
	{

	}

	QString MessageAlertWidget::id() const
	{
		return State_.AimId_;
	}

    QString MessageAlertWidget::mailId() const
    {
        return State_.MailId_;
    }

	void MessageAlertWidget::paintEvent(QPaintEvent*)
	{
        if (!Painter_)
        {
            Painter_ = new QPainter(this);
            Delegate_->paint(Painter_, Options_, State_, false);
            Painter_->end();
            return;
        }

        if (Painter_->begin(this))
        {
			Delegate_->paint(Painter_, Options_, State_, false);
			Painter_->end();
		}
	}

	void MessageAlertWidget::resizeEvent(QResizeEvent* e)
	{
		Options_.initFrom(this);
		return QWidget::resizeEvent(e);
	}

	void MessageAlertWidget::enterEvent(QEvent* e)
	{
		Options_.state = Options_.state | QStyle::State_MouseOver;
		update();
		return QWidget::enterEvent(e);
	}

	void MessageAlertWidget::leaveEvent(QEvent* e)
	{
		Options_.state = Options_.state & ~QStyle::State_MouseOver;
		update();
		return QWidget::leaveEvent(e);
	}

    void MessageAlertWidget::mousePressEvent(QMouseEvent* e)
    {
        e->accept();
    }

	void MessageAlertWidget::mouseReleaseEvent(QMouseEvent *e)
	{
        e->accept();
		emit clicked(State_.AimId_, State_.MailId_);
	}

    void MessageAlertWidget::avatarChanged(QString aimId)
    {
        if (aimId != State_.AimId_ && State_.Friendly_.indexOf(aimId) == -1)
            return;

        update();
    }
}