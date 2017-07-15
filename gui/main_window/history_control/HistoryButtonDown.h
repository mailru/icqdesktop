#pragma once

#include "../../namespaces.h"
#include "../../controls/CustomButton.h"

// Button with new messages bubbles to history

UI_NS_BEGIN

class HistoryButtonDown final : public CustomButton
{
    Q_OBJECT

protected:
    virtual void paintEvent(QPaintEvent *_event) override;
    virtual void leaveEvent(QEvent *_event) override;
    virtual void enterEvent(QEvent *_event) override;
    virtual void mousePressEvent(QMouseEvent *_event) override;
    virtual void mouseReleaseEvent(QMouseEvent *_event) override;
    virtual void wheelEvent(QWheelEvent * _event) override;

public:
	HistoryButtonDown(QWidget* _parent, const QString& _imageName);
    HistoryButtonDown(QWidget* _parent, const QPixmap& _pixmap);
	
	void addUnreadMessages(int num_add);
	void setUnreadMessages(int num_unread);

private:
	int numUnreads_;

Q_SIGNALS:
    void sendWheelEvent(QWheelEvent* e);
};

UI_NS_END