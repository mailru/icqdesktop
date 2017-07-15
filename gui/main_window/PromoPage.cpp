//
//  PromoPage.cpp
//  ICQ
//
//  Created by g.ulyanov on 24/05/16.
//  Copyright © 2016 Mail.RU. All rights reserved.
//

#include "stdafx.h"
#include "PromoPage.h"
#include "MainWindow.h"
#include "../core_dispatcher.h"
#include "../controls/CommonStyle.h"
#include "../controls/TextEmojiWidget.h"
#include "../utils/InterConnector.h"
#include "../utils/utils.h"

namespace Ui
{
    namespace
    {
        const int dotSize = 10;
        
        struct Page
        {
            QString image_;
            QString topic_;
            QString text1_;
            QString text2_;
            QString next_;
            QString skip_;
            Page(const QString &image, const QString &topic, const QString &text1, const QString &text2, const QString &next, const QString &skip):
                image_(image), topic_(topic), text1_(text1), text2_(text2), next_(next), skip_(skip)
            {
            }
        };
        std::vector<Page> pages;
        size_t currentPage = 0;
    }
    
    void PromoImage::resizeEvent(QResizeEvent *e)
    {
        QPushButton::resizeEvent(e);
    }
    
    void PromoImage::paintEvent(QPaintEvent * /*e*/)
    {
        if (!image_.isNull())
        {
            QPainter painter(this);
            
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            const auto w = (image_.size().width() * (parentWidget()->height() / (float)image_.size().height()));
            painter.drawImage(QRect((parentWidget()->width() - w) / 2., 0, w, parentWidget()->height()), image_);
        }
    }
    
    PromoImage::PromoImage(QWidget *parent/* = nullptr*/): QPushButton(parent)
    {
        //
    }
    
    PromoImage::~PromoImage()
    {
        //
    }
    
    void PromoImage::setImage(const QImage &image)
    {
        image_ = image;
        update();
    }
    
    void PromoPageDot::paintEvent(QPaintEvent * /*e*/)
    {
        QPainter painter(this);

        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        if (isChecked())
        {
            auto thickness = Utils::scale_value(2);
            painter.setPen(QPen(QBrush("#579e1c"), thickness));
            painter.setBrush(QBrush("#f8f8f8"));
            painter.drawEllipse(thickness / 2, thickness / 2, width() - thickness, height() - thickness);
        }
        else
        {
            painter.setPen(Qt::transparent);
            painter.setBrush(QBrush("#d7d7d7"));
            painter.drawEllipse(0, 0, width(), height());
        }
    }
    
    PromoPageDot::PromoPageDot(QWidget *parent/* = nullptr*/): QCheckBox(parent)
    {
        setCursor(Qt::CursorShape::PointingHandCursor);
    }

    PromoPageDot::~PromoPageDot()
    {
        //
    }

    PromoPage::PromoPage(QWidget *parent/* = nullptr*/): QWidget(parent)
    {
        pages.emplace_back(Page(":/resources/promo_pages/coach_1_200.jpg",
                                QT_TRANSLATE_NOOP("promo_page", "Contacts"),
                                QT_TRANSLATE_NOOP("promo_page", "The contact list is now"),
                                QT_TRANSLATE_NOOP("promo_page", "located here"),
                                QT_TRANSLATE_NOOP("promo_page", "Next"),
                                QT_TRANSLATE_NOOP("promo_page", "Skip")));
        pages.emplace_back(Page(":/resources/promo_pages/coach_2_200.jpg",
                                QT_TRANSLATE_NOOP("promo_page", "Synchronization"),
                                QT_TRANSLATE_NOOP("promo_page", "Synchronize your chat history"),
                                QT_TRANSLATE_NOOP("promo_page", "and contact list across all devices"),
                                QT_TRANSLATE_NOOP("promo_page", "Next"),
                                QT_TRANSLATE_NOOP("promo_page", "Skip")));
        pages.emplace_back(Page(":/resources/promo_pages/coach_3_200.jpg",
                                QT_TRANSLATE_NOOP("promo_page", "Stickers"),
                                QT_TRANSLATE_NOOP("promo_page", "Click here and choose"),
                                QT_TRANSLATE_NOOP("promo_page", "from over 500 bright stickers"),
                                QT_TRANSLATE_NOOP("promo_page", "Next"),
                                QT_TRANSLATE_NOOP("promo_page", "Skip")));
        pages.emplace_back(Page(":/resources/promo_pages/coach_4_200.jpg",
                                QT_TRANSLATE_NOOP("promo_page", "Wallpapers"),
                                QT_TRANSLATE_NOOP("promo_page", "Customize your chats"),
                                QT_TRANSLATE_NOOP("promo_page", "Choose a wallpaper from our collection"),
                                QT_TRANSLATE_NOOP("promo_page", "Get Started"),
                                ""));

        PromoImage *promoImage = nullptr;
        std::vector<PromoPageDot *> dots;
        TextEmojiWidget *promoTopic = nullptr;
        TextEmojiWidget *promoText1 = nullptr;
        TextEmojiWidget *promoText2 = nullptr;
        QPushButton *nextButton = nullptr;
        QPushButton *skipButton = nullptr;
        
        auto layout = Utils::emptyVLayout(this);
        layout->setAlignment(Qt::AlignCenter);
        
        auto content = new QWidget(this);
        auto contentLayout = Utils::emptyVLayout(content);
        contentLayout->setAlignment(Qt::AlignBottom | Qt::AlignCenter);
        content->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        content->setStyleSheet("background-color: #ebebeb;");
        {
            promoImage = new PromoImage(content);
            Utils::ApplyStyle(promoImage, QString("padding:0; margin:0; border-image:url();"));
            promoImage->setImage(QImage(pages[currentPage].image_));
            //promoImage->setFixedSize(Utils::scale_value(640), Utils::scale_value(328));
            promoImage->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            contentLayout->addWidget(promoImage);
        }
        layout->addWidget(content);
        
        auto controls = new QWidget(this);
        auto controlsLayout = Utils::emptyVLayout(controls);
        controlsLayout->setContentsMargins(0, Utils::scale_value(16), 0, Utils::scale_value(24));
        controlsLayout->setAlignment(Qt::AlignTop | Qt::AlignCenter);
        controls->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);
        controls->setStyleSheet("background-color:#f8f8f8;");
        {
            auto navigationDots = new QWidget(controls);
            auto navigationDotsLayout = new QHBoxLayout(navigationDots);
            navigationDots->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
            navigationDotsLayout->setSpacing(Utils::scale_value(20));
            navigationDotsLayout->setMargin(0);
            navigationDotsLayout->setAlignment(Qt::AlignCenter);
            for (size_t i = 0, iend = pages.size(); i < iend; ++i)
            {
                auto dot = new PromoPageDot(navigationDots);
                dot->setFixedSize(Utils::scale_value(dotSize), Utils::scale_value(dotSize));
                dot->setChecked(i == currentPage);
                navigationDotsLayout->addWidget(dot);
                dots.push_back(dot);
            }
            controlsLayout->addWidget(navigationDots);
        }
        {
            auto hollow = new QWidget(controls);
            hollow->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            hollow->setMinimumHeight(Utils::scale_value(24));
            controlsLayout->addWidget(hollow);
        }
        {
            auto promoDescription = new QWidget(controls);
            auto promoDescriptionLayout = Utils::emptyVLayout(promoDescription);
            promoDescriptionLayout->setAlignment(Qt::AlignCenter);
            promoDescription->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
            {
                auto sub = new QWidget(promoDescription);
                auto subLayout = Utils::emptyVLayout(sub);
                subLayout->setAlignment(Qt::AlignCenter);
                sub->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
                {
                    promoTopic = new TextEmojiWidget(sub, Fonts::appFontScaled(24), CommonStyle::getTextCommonColor(), Utils::scale_value(26));
                    promoTopic->setSizePolicy(QSizePolicy::Policy::Preferred, promoTopic->sizePolicy().verticalPolicy());
                    promoTopic->setText(pages[currentPage].topic_);
                    subLayout->addWidget(promoTopic);
                }
                promoDescriptionLayout->addWidget(sub);
            }
            {
                auto sub = new QWidget(promoDescription);
                auto subLayout = Utils::emptyVLayout(sub);
                subLayout->setAlignment(Qt::AlignCenter);
                sub->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
                {
                    promoText1 = new TextEmojiWidget(sub, Fonts::appFontScaled(16), QColor("#767676"), Utils::scale_value(36));
                    promoText1->setSizePolicy(QSizePolicy::Policy::Preferred, promoText1->sizePolicy().verticalPolicy());
                    promoText1->setText(pages[currentPage].text1_);
                    subLayout->addWidget(promoText1);
                }
                promoDescriptionLayout->addWidget(sub);
            }
            {
                auto sub = new QWidget(promoDescription);
                auto subLayout = Utils::emptyVLayout(sub);
                subLayout->setAlignment(Qt::AlignCenter);
                sub->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
                {
                    promoText2 = new TextEmojiWidget(sub, Fonts::appFontScaled(16), QColor("#767676"), Utils::scale_value(18));
                    promoText2->setSizePolicy(QSizePolicy::Policy::Preferred, promoText2->sizePolicy().verticalPolicy());
                    promoText2->setText(pages[currentPage].text2_);
                    subLayout->addWidget(promoText2);
                }
                promoDescriptionLayout->addWidget(sub);
            }
            {
                auto hollow = new QWidget(promoDescription);
                hollow->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                hollow->setFixedHeight(Utils::scale_value(24));
                promoDescriptionLayout->addWidget(hollow);
            }
            {
                auto sub = new QWidget(promoDescription);
                auto subLayout = Utils::emptyVLayout(sub);
                subLayout->setAlignment(Qt::AlignCenter);
                sub->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
                {
                    nextButton = new QPushButton(sub);
                    nextButton->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
                    nextButton->setFlat(true);
                    nextButton->setText(pages[currentPage].next_);
                    nextButton->setCursor(Qt::PointingHandCursor);
                    Utils::ApplyStyle(nextButton, CommonStyle::getGreenButtonStyle());
                    subLayout->addWidget(nextButton);
                }
                promoDescriptionLayout->addWidget(sub);
            }
            controlsLayout->addWidget(promoDescription);
        }
        {
            auto hollow = new QWidget(controls);
            hollow->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            hollow->setMinimumHeight(Utils::scale_value(24));
            controlsLayout->addWidget(hollow);
        }
        {
            auto sub = new QWidget(controls);
            auto subLayout = Utils::emptyVLayout(sub);
            subLayout->setAlignment(Qt::AlignCenter);
            sub->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
            {
                skipButton = new QPushButton(sub);
                skipButton->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
                skipButton->setFlat(true);
                skipButton->setText(pages[currentPage].skip_);
                skipButton->setCursor(Qt::PointingHandCursor);
                Utils::ApplyStyle(skipButton, "background-color:transparent; border-style:none; font-size:16dip; color:#767676; padding:0; margin:0;");
                subLayout->addWidget(skipButton);
            }
            controlsLayout->addWidget(sub);
        }
        layout->addWidget(controls);
        
        auto updateRoutine = [=]()
        {
            promoImage->setImage(QImage(pages[currentPage].image_));
            for (size_t i = 0, iend = dots.size(); i < iend; ++i)
            {
                dots[i]->setChecked(i == currentPage);
            }
            promoTopic->setText(pages[currentPage].topic_);
            promoText1->setText(pages[currentPage].text1_);
            promoText2->setText(pages[currentPage].text2_);
            promoText2->setSizeToBaseline(promoText2->text().trimmed().isEmpty() ? Utils::scale_value(2) : Utils::scale_value(18));
            nextButton->setText(pages[currentPage].next_);
            skipButton->setText(pages[currentPage].skip_);
            skipButton->setEnabled(!skipButton->text().isEmpty());
        };
        
        for (size_t i = 0, iend = dots.size(); i < iend; ++i)
        {
            QObject::connect(dots[i], &PromoPageDot::clicked, [=]()
            {
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::promo_switch);
                currentPage = i;
                updateRoutine();
            });
        }
        QObject::connect(nextButton, &QPushButton::clicked, [=]()
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::promo_next);
            if ((currentPage + 1) < pages.size())
            {
                currentPage++;
                updateRoutine();
            }
            else
            {
                emit Utils::InterConnector::instance().getMainWindow()->closePromoPage();
            }
        });
        QObject::connect(skipButton, &QPushButton::clicked, [=]()
        {
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::promo_skip);
            emit Utils::InterConnector::instance().getMainWindow()->closePromoPage();
        });
    }
    
    PromoPage::~PromoPage()
    {
        //
    }
}
