#include "stdafx.h"
#include "GeneralSettingsWidget.h"
#include "../LoginPage.h"
#include "../../core_dispatcher.h"
#include "../../controls/GeneralCreator.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"

using namespace Ui;

void GeneralSettingsWidget::Creator::initAttachUin(QWidget* _parent, std::map<std::string, Synchronizator> &/*collector*/)
{
    auto scrollArea = new QScrollArea(_parent);
    scrollArea->setWidgetResizable(true);
    Utils::grabTouchWidget(scrollArea->viewport(), true);

    auto mainWidget = new QWidget(scrollArea);
    Utils::grabTouchWidget(mainWidget);

    auto mainLayout = Utils::emptyVLayout(mainWidget);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(Utils::scale_value(36), 0, Utils::scale_value(36), Utils::scale_value(36));

    scrollArea->setWidget(mainWidget);

    auto layout = Utils::emptyHLayout(_parent);
    
    LoginPage* page = new LoginPage(scrollArea, false /* is_login */);
    page->switchLoginType();

    GeneralCreator::addBackButton(scrollArea, layout, [page]()
    {
        page->prevPage();
        page->switchLoginType();
        emit Utils::InterConnector::instance().attachUinBack(); 
    });
    layout->addWidget(scrollArea);

    GeneralCreator::addHeader(scrollArea, mainLayout, QT_TRANSLATE_NOOP("sidebar", "Connect to ICQ account"));

    mainLayout->addWidget(page);

    connect(page, &LoginPage::attached, [page]() 
    {
        if (!page->isVisible())
            return;

        core::coll_helper helper(GetDispatcher()->create_collection(), true);
        GetDispatcher()->post_message_to_core("load_flags", helper.get());
        emit Utils::InterConnector::instance().attachUinBack();
        emit Utils::InterConnector::instance().profileSettingsUpdateInterface();
    });

    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::updateFocus, page, &LoginPage::updateFocus);
}
