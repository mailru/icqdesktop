#include "stdafx.h"
#include "SearchFilters.h"
#include "ComboButton.h"
#include "../../utils/utils.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/LineEditEx.h"
#include "../../fonts.h"

namespace Ui
{
    SearchFilters::SearchFilters(QWidget* _parent)
        :	QWidget(_parent)
        , keyword_(new LineEditEx(this))
        , searchButton_(new QPushButton(this))

    {

        QVBoxLayout* rootLayout = Utils::emptyVLayout();
        rootLayout->setContentsMargins(0, Utils::scale_value(16), 0, 0);

        QHBoxLayout* keywordLayout = Utils::emptyHLayout();

        keyword_->setPlaceholderText(QT_TRANSLATE_NOOP("search_widget", "Phone or Name, Email, UIN"));
        keyword_->setAttribute(Qt::WA_MacShowFocusRect, false);
        keyword_->setObjectName("keyword");
        keyword_->setFont(Fonts::appFontScaled(18));
        QString keyword_style;
        keyword_style =
            "QLineEdit { margin-right: 12dip; padding: 0; }";
        Utils::ApplyStyle(keyword_, Ui::CommonStyle::getLineEditStyle() + keyword_style);

        keywordLayout->addWidget(keyword_);

		Utils::ApplyStyle(searchButton_, CommonStyle::getGreenButtonStyle());
        searchButton_->setText(QT_TRANSLATE_NOOP("search_widget", "Search"));
        searchButton_->setObjectName("search_button");
        searchButton_->setCursor(QCursor(Qt::PointingHandCursor));

        keywordLayout->addWidget(searchButton_);

        rootLayout->addLayout(keywordLayout);

        /*	QHBoxLayout* filtersLayout = new QHBoxLayout();
        filtersLayout->setAlignment(Qt::AlignLeft);
        filtersLayout->setContentsMargins(0, Utils::scale_value(16), 0, 0);

        ComboButton* buttonGender = new ComboButton(this);
        buttonGender->setText(TR_("SearchFilters", "Gender"));
        filtersLayout->addWidget(buttonGender);

        ComboButton* buttonCountry = new ComboButton(this);
        buttonCountry->setText(TR_("SearchFilters", "Country"));
        filtersLayout->addWidget(buttonCountry);

        ComboButton* buttonAge = new ComboButton(this);
        buttonAge->setText(TR_("SearchFilters", "Age"));
        filtersLayout->addWidget(buttonAge);

        QCheckBox* onlineCheck = new QCheckBox(TR_("SearchFilters", "Online"), this);
        onlineCheck->setObjectName("onlineCheck");
        filtersLayout->addWidget(onlineCheck);

        rootLayout->addLayout(filtersLayout);*/

        setLayout(rootLayout);


        connect(searchButton_, SIGNAL(clicked()), this, SLOT(onSearchButtonClicked()), Qt::QueuedConnection);
        connect(keyword_, SIGNAL(enter()), this, SLOT(onSearchButtonClicked()), Qt::QueuedConnection);
        connect(keyword_, SIGNAL(clicked()), this, SIGNAL(clicked()), Qt::QueuedConnection);
    }

    SearchFilters::~SearchFilters()
    {
    }

    void SearchFilters::onSearchResults()
    {
        searchButton_->setEnabled(true);
    }

    void SearchFilters::onFocus()
    {
        keyword_->setFocus(Qt::FocusReason::MouseFocusReason);
    }

    void SearchFilters::onSearchButtonClicked()
    {
        searchButton_->setEnabled(false);

        search_params filters;
        filters.setKeyword(keyword_->text().trimmed());

        emit onSearch(filters);
    }

    void SearchFilters::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }
}
