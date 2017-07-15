#include "stdafx.h"
#include "SearchWidget.h"
#include "../../controls/LineEditEx.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/CustomButton.h"
#include "../../fonts.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"

namespace Ui
{
    SearchWidget::SearchWidget(QWidget* _parent, int _add_hor_space, int _add_ver_space)
        : QWidget(_parent)
    {
        setFixedHeight(Utils::scale_value(28) + _add_ver_space * 2);
        active_ = false;
        vMainLayout_ = Utils::emptyVLayout(this);
        vMainLayout_->setContentsMargins(Utils::scale_value(12) + _add_hor_space, _add_ver_space, Utils::scale_value(12) + _add_hor_space, _add_ver_space);
        hMainLayout = Utils::emptyHLayout();

        searchWidget_ = new QWidget(this);

        hSearchLayout_ = Utils::emptyHLayout(searchWidget_);
        hSearchLayout_->setContentsMargins(Utils::scale_value(2), 0, Utils::scale_value(2), 0);

        searchEdit_ = new LineEditEx(this);
        searchEdit_->setFixedHeight(Utils::scale_value(24));
        searchEdit_->setFont(Fonts::appFontScaled(15));
        searchEdit_->setFrame(QFrame::NoFrame);
        searchEdit_->setPlaceholderText(QT_TRANSLATE_NOOP("search_widget", "Search"));
        searchEdit_->setContentsMargins(Utils::scale_value(8), 0, 0, 0);
        searchEdit_->setAttribute(Qt::WA_MacShowFocusRect, false);
        Testing::setAccessibleName(searchEdit_, "search_edit");
        hSearchLayout_->addSpacing(0);
        hSearchLayout_->addWidget(searchEdit_);

        closeIcon_ = new Ui::CustomButton(this, ":/resources/basic_elements/close_a_100.png");
        closeIcon_->setActiveImage(":/resources/basic_elements/close_d_100.png");
        closeIcon_->setHoverImage(":/resources/basic_elements/close_d_100.png");
        closeIcon_->setFixedSize(Utils::scale_value(28), Utils::scale_value(24));
        closeIcon_->setStyleSheet("border: none;");
        closeIcon_->setFocusPolicy(Qt::NoFocus);
        closeIcon_->setCursor(Qt::PointingHandCursor);
        hSearchLayout_->addWidget(closeIcon_);
        closeIcon_->hide();

        hMainLayout->addWidget(searchWidget_);
        vMainLayout_->addLayout(hMainLayout);

        auto style = QString("border-style: solid; border-width: 1dip; border-color: #cbcbcb; border-radius: 4dip; background: #ffffff;");
        Utils::ApplyStyle(this, style);
        searchEdit_->setStyleSheet("border: none; background: transparent;");

        QMetaObject::connectSlotsByName(this);
        connect(searchEdit_, SIGNAL(textEdited(QString)), this, SLOT(searchChanged(QString)), Qt::QueuedConnection);
        connect(searchEdit_, SIGNAL(clicked()), this, SLOT(searchStarted()), Qt::QueuedConnection);
        connect(searchEdit_, SIGNAL(escapePressed()), this, SLOT(onEscapePress()), Qt::QueuedConnection);
        connect(searchEdit_, SIGNAL(enter()), this, SLOT(editEnterPressed()), Qt::QueuedConnection);
        connect(searchEdit_, SIGNAL(upArrow()), this, SLOT(editUpPressed()), Qt::QueuedConnection);
        connect(searchEdit_, SIGNAL(downArrow()), this, SLOT(editDownPressed()), Qt::QueuedConnection);
        connect(searchEdit_, SIGNAL(focusOut()), this, SLOT(focusedOut()), Qt::QueuedConnection);
        connect(closeIcon_, SIGNAL(clicked()), this, SLOT(searchCompleted()), Qt::QueuedConnection);

        setActive(false);
    }

    void SearchWidget::setFocus()
    {
        searchEdit_->setFocus(Qt::MouseFocusReason);
        setActive(true);
    }

    void SearchWidget::clearFocus()
    {
        searchEdit_->clearFocus();
        setActive(false);
    }

    SearchWidget::~SearchWidget()
    {
    }

    void SearchWidget::setActive(bool _active)
    {
        if (active_ == _active)
            return;

        auto style = QString("border-style: solid; border-width: 1dip; border-color: %1; border-radius: 4dip; background: #ffffff;").arg(_active ? "#579e1c" : "#cbcbcb");
        Utils::ApplyStyle(this, style);
        
        active_ = _active;
        closeIcon_->setVisible(_active);
        emit activeChanged(_active);
    }

    void SearchWidget::searchStarted()
    {
        setActive(true);
    }

    QString SearchWidget::getText() const
    {
        return searchEdit_->text();
    }

    bool SearchWidget::isEmpty() const
    {
        return getText().isEmpty();
    }

    void SearchWidget::clearInput()
    {
        searchEdit_->clear();
    }

    void SearchWidget::searchCompleted()
    {
        clearInput();
        setActive(false);
        searchEdit_->clearFocus();
        emit searchEnd();
        Utils::InterConnector::instance().setFocusOnInput();
    }

    void SearchWidget::onEscapePress()
    {
        searchCompleted();
        emit escapePressed();
    }

    void SearchWidget::searchChanged(QString _text)
    {
        setActive(true);

        if (!_text.isEmpty())
        {
            emit searchBegin();
        }
        else
        {
            emit inputEmpty();
        }

        emit search(platform::is_apple() ? _text.toLower() : _text);
    }

    void SearchWidget::editEnterPressed()
    {
        emit enterPressed();
    }

    void SearchWidget::editUpPressed()
    {
        emit upPressed();
    }

    void SearchWidget::editDownPressed()
    {
        emit downPressed();
    }

    void SearchWidget::focusedOut()
    {
        //setActive(false);
    }
}
