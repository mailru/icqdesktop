#include "stdafx.h"

#include "../../../core_dispatcher.h"
#include "../../../controls/CommonStyle.h"
#include "../../../controls/LabelEx.h"
#include "../../../fonts.h"
#include "../../../main_window/ContactDialog.h"
#include "../../../main_window/GroupChatOperations.h"
#include "../../../utils/utils.h"
#include "../../../utils/gui_coll_helper.h"
#include "../../contact_list/ContactList.h"
#include "../../contact_list/ContactListModel.h"
#include "../../contact_list/SelectionContactsForGroupChat.h"
#include "../../MainPage.h"
#include "../../history_control/MessagesScrollArea.h"

#include "SelectionPanel.h"

namespace
{
    const int horizontalSpace = 24;
    const int verticalSpace = 15;
    const int buttonSpace = 12;
}

Ui::Selection::SelectionPanel::SelectionPanel(MessagesScrollArea* _messages, QWidget* _parent)
    : QFrame(_parent)
    , messages_(_messages)
{
    assert(_messages);

    const auto style = 
        Utils::LoadStyle(":/main_window/history_control/history_control.qss");
    setStyleSheet(style);
    setObjectName("topWidget");

    auto forward = new QPushButton(QT_TRANSLATE_NOOP("chat_page", "Forward"));
    forward->setCursor(Qt::PointingHandCursor);
    Utils::ApplyStyle(forward, Ui::CommonStyle::getGreenButtonStyle());

    auto copy = new QPushButton(QT_TRANSLATE_NOOP("chat_page", "Copy"));
    copy->setCursor(Qt::PointingHandCursor);
    Utils::ApplyStyle(copy, Ui::CommonStyle::getGreenButtonStyle());

    auto cancel = new LabelEx(this);
    cancel->setText(QT_TRANSLATE_NOOP("chat_page", "Cancel"));
    QPalette p;
    p.setColor(QPalette::Foreground, CommonStyle::getLinkColor());
    cancel->setPalette(p);
    cancel->setFont(Fonts::appFontScaled(16));
    cancel->setCursor(Qt::PointingHandCursor);
    cancel->adjustSize();

    connect(forward, &QPushButton::clicked, this, [=]()
    {
        auto quotes = messages_->getQuotes();
        if (quotes.empty())
            return;

        if (quotes.size() == 1 && 
            (quotes.begin()->type_ == Data::Quote::Type::file_sharing ||
            quotes.begin()->type_ == Data::Quote::Type::link))
        {
            forwardMessage(quotes.begin()->text_, false);
        }
        else
        {
            forwardMessage(quotes, false);
        }

        closePanel();
    });

    connect(copy, &QPushButton::clicked, [this]()
    {
        const auto text = messages_->getSelectedText();;

        auto clipboard = QApplication::clipboard();
        clipboard->setText(text);

        closePanel();

        Ui::MainPage::instance()->getContactDialog()->setFocusOnInputWidget();
    });

    connect(cancel, &LabelEx::clicked, [this]()
    {
        closePanel();
    });

    auto layout = Utils::emptyHLayout();
    layout->setContentsMargins(
        Utils::scale_value(horizontalSpace), Utils::scale_value(verticalSpace),
        Utils::scale_value(horizontalSpace), Utils::scale_value(verticalSpace));

    layout->addWidget(forward);
    layout->addSpacing(Utils::scale_value(buttonSpace));
    layout->addWidget(copy);
    layout->addStretch();
    layout->addWidget(cancel);

    setLayout(layout);
}

void Ui::Selection::SelectionPanel::closePanel()
{
    messages_->clearSelection();
}
