#include "stdafx.h"
#include "ThemesPage.h"

#include "ThemeWidget.h"
#include "ThemesModel.h"
#include "../../../theme_settings.h"
#include "../../../controls/FlowLayout.h"
#include "../../../controls/TransparentScrollBar.h"
#include "../../../controls/TextEmojiWidget.h"
#include "../../../controls/CommonStyle.h"
#include "../../../controls/BackButton.h"

#include "../../contact_list/RecentsModel.h"
#include "../../../utils/InterConnector.h"
#include "../../../utils/utils.h"

namespace Ui
{

    ThemesPage::ThemesPage(QWidget* _parent, int _spacing)
        : QWidget(_parent)
        , themesModel_(new ThemesModel(this))
        , flowLayout_(new FlowLayout(0, _spacing, _spacing))
        , themesList_(themes::themesList())
        , loadedThemes_(QMap<int, bool>())
        , firstThemeAdded_(false)
    {
        QVBoxLayout *rootLayout = Utils::emptyVLayout();
        auto area = CreateScrollAreaAndSetTrScrollBar(this);
        QWidget* scrollArea = new QWidget(area);
        Utils::grabTouchWidget(area->viewport(), true);
        Utils::grabTouchWidget(area);
        Utils::grabTouchWidget(scrollArea);
        connect(QScroller::scroller(area->viewport()), SIGNAL(stateChanged(QScroller::State)), this, SLOT(touchScrollStateChanged(QScroller::State)), Qt::QueuedConnection);

        area->setWidget(scrollArea);
        area->setWidgetResizable(true);
        auto layout = Utils::emptyVLayout();
        layout->setContentsMargins(Utils::scale_value(36), 0, Utils::scale_value(36), Utils::scale_value(36));
        layout->setAlignment(Qt::AlignTop);
        scrollArea->setLayout(layout);
        rootLayout->addWidget(area);
        setLayout(rootLayout);

        auto themesCaption = new TextEmojiWidget(this, Fonts::appFontScaled(20, Fonts::FontWeight::Medium), Ui::CommonStyle::getTextCommonColor(), Utils::scale_value(36));
        themesCaption->setText(QT_TRANSLATE_NOOP("settings_pages", "Wallpaper"));
        Utils::grabTouchWidget(themesCaption);

        backButton_ = new BackButton(this);
        backButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        backButton_->setFlat(true);
        backButton_->setFocusPolicy(Qt::NoFocus);
        backButton_->setCursor(Qt::PointingHandCursor);

        connect(backButton_, SIGNAL(clicked()), this, SLOT(backPressed()));

        auto themeCaptionLayout_ = Utils::emptyHLayout();
        themeCaptionLayout_->setContentsMargins(0, 0, 0, Utils::scale_value(16));
        themeCaptionLayout_->addWidget(backButton_);

        backButtonAndCaptionSpacer_ = new QWidget(this);
        backButtonAndCaptionSpacer_->setFixedSize(Utils::scale_value(16), 1);
        Utils::grabTouchWidget(backButtonAndCaptionSpacer_);
        themeCaptionLayout_->addWidget(backButtonAndCaptionSpacer_);
        themeCaptionLayout_->addWidget(themesCaption);

        layout->addLayout(themeCaptionLayout_);
        layout->addLayout(flowLayout_);
    }

    ThemesPage::~ThemesPage()
    {
    }

    void ThemesPage::setBackButton(bool _doSet)
    {
        backButton_->setVisible(_doSet);
        backButtonAndCaptionSpacer_->setVisible(_doSet);
    }

    void ThemesPage::resizeEvent(QResizeEvent* _e)
    {
        QWidget::resizeEvent(_e);
    }

    void ThemesPage::touchScrollStateChanged(QScroller::State _st)
    {
        for (int i = 0; i < flowLayout_->count(); ++i)
        {
            QLayoutItem *layoutItem = flowLayout_->itemAt(i);
            if (QWidget* w = layoutItem->widget())
            {
                w->setAttribute(Qt::WA_TransparentForMouseEvents, _st != QScroller::Inactive);
            }
        }
    }

    void ThemesPage::onThemeGot(themes::themePtr _theme)
    {
        checkFirstTheme_(_theme);
        int themeId = _theme->get_id();
        bool themeLoaded = loadedThemes_[themeId];
        if (!themeLoaded)
        {
            QPixmap p = _theme->getThumb();
            ThemeWidget *themeWidget = new ThemeWidget(this, p, themesModel_, _theme->get_id());
            Utils::grabTouchWidget(themeWidget);
            flowLayout_->addWidget(themeWidget);
            update();
            loadedThemes_[themeId] = true;
        }
    }

    void ThemesPage::setTargetContact(QString _aimId)
    {
        themesModel_->setTargetContact(_aimId);
        int themeId = get_qt_theme_settings()->themeIdForContact(_aimId);

        for (int i = 0; i < flowLayout_->count(); ++i)
        {
            QLayoutItem *layoutItem = flowLayout_->itemAt(i);
            if (layoutItem->widget())
            {
                ThemeWidget *themeWidget = qobject_cast<ThemeWidget *>(layoutItem->widget());
                bool shouldSelect = themeWidget->get_id() == themeId;
                themeWidget->setBorder(shouldSelect);
            }
        }
    }

    void ThemesPage::checkFirstTheme_(themes::themePtr _theme)
    {
        if (!firstThemeAdded_)
        {
            firstThemeAdded_ = true;

            QSize properSize = _theme->getThumb().size();
            auto initialTheme = std::make_shared<themes::theme>();
            auto initialThemeThumb = initialTheme->getThumb();
            auto correctedInitialThemeThumb = initialThemeThumb.scaled(properSize, Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);
            initialTheme->setThumb(correctedInitialThemeThumb);
            onThemeGot(initialTheme);
        }
    }

    void ThemesPage::backPressed()
    {
        Logic::getRecentsModel()->sendLastRead();
        emit Utils::InterConnector::instance().themesSettingsBack();
    }
};
