#include "stdafx.h"
#include "GeneralCreator.h"

#include "BackButton.h"
#include "CommonStyle.h"
#include "FlatMenu.h"
#include "TextEmojiWidget.h"
#include "../core_dispatcher.h"
#include "../utils/utils.h"

namespace
{
    const QString DROPDOWN_STYLE =
        "border-width: 1dip; border-bottom-style: solid; border-color: #d7d7d7; ";
}

namespace Ui
{
    void GeneralCreator::addHeader(QWidget* _parent, QLayout* _layout, const QString& _text)
    {
        auto title = new TextEmojiWidget(
            _parent,
            Fonts::appFontScaled(20, Fonts::FontWeight::Medium),
            Ui::CommonStyle::getTextCommonColor(),
            Utils::scale_value(36));

        title->setText(_text);
        _layout->addWidget(title);
        Utils::grabTouchWidget(title);
    }

    GeneralCreator::addSwitcherWidgets GeneralCreator::addSwitcher(std::map<std::string, Synchronizator>* _collector, QWidget* _parent, QLayout* _layout, const QString& _text, bool _switched, std::function< QString(bool) > _slot)
    {
        addSwitcherWidgets switcherWidget;

        auto mainWidget = new QWidget(_parent);
        auto mainLayout = Utils::emptyHLayout(mainWidget);
        mainLayout->setAlignment(Qt::AlignLeft);

        Utils::grabTouchWidget(mainWidget);

        auto text = new TextEmojiWidget(mainWidget, Fonts::appFontScaled(16), Ui::CommonStyle::getTextCommonColor(), Utils::scale_value(44));
        switcherWidget.text_ = text;
        Utils::grabTouchWidget(text);
        text->setFixedWidth(Utils::scale_value(312));
        text->setText(_text);
        text->setMultiline(true);
        mainLayout->addWidget(text);

        auto checkboxWidget = new QWidget(_parent);
        Utils::grabTouchWidget(checkboxWidget);
        auto checkboxLayout = Utils::emptyVLayout(checkboxWidget);
        checkboxWidget->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
        checkboxLayout->setAlignment(Qt::AlignBottom);
        {
            auto checkbox = new QCheckBox(checkboxWidget);
            switcherWidget.check_ = checkbox;
            checkbox->setObjectName("greenSwitcher");
            checkbox->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
            checkbox->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
            checkbox->setChecked(_switched);

            auto palette = checkbox->palette();
            checkbox->setPalette(palette);

            GetDispatcher()->set_enabled_stats_post(false);
            checkbox->setText(_slot(checkbox->isChecked()));
            GetDispatcher()->set_enabled_stats_post(true);

            if (_collector)
            {
                auto it = _collector->find("switcher/switch");
                if (it != _collector->end())
                    it->second.widgets_.push_back(checkbox);
                else
                    _collector->insert(std::make_pair("switcher/switch", Synchronizator(checkbox, SIGNAL(pressed()), SLOT(toggle()))));
            }
            QObject::connect(checkbox, &QCheckBox::clicked, [checkbox, _slot]()
            {
                checkbox->setText(_slot(checkbox->isChecked()));
            });
            checkboxLayout->addWidget(checkbox);
        }
        mainLayout->addWidget(checkboxWidget);

        _layout->addWidget(mainWidget);

        return switcherWidget;
    }

    TextEmojiWidget* GeneralCreator::addChooser(QWidget* _parent, QLayout* _layout, const QString& _info, const QString& _value, std::function< void(TextEmojiWidget*) > _slot)
    {
        auto mainWidget = new QWidget(_parent);
        auto mainLayout = Utils::emptyHLayout(mainWidget);
        mainLayout->setAlignment(Qt::AlignLeft);

        Utils::grabTouchWidget(mainWidget);

        auto info = new TextEmojiWidget(mainWidget, Fonts::appFontScaled(16), Ui::CommonStyle::getTextCommonColor(), Utils::scale_value(44));
        Utils::grabTouchWidget(info);
        info->setSizePolicy(QSizePolicy::Policy::Preferred, info->sizePolicy().verticalPolicy());
        info->setText(_info);
        mainLayout->addWidget(info);

        auto valueWidget = new QWidget(_parent);
        auto valueLayout = Utils::emptyVLayout(valueWidget);
        Utils::grabTouchWidget(valueWidget);
        valueWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        valueLayout->setAlignment(Qt::AlignBottom);
        valueLayout->setContentsMargins(Utils::scale_value(8), 0, 0, 0);

        auto value = new TextEmojiWidget(valueWidget, Fonts::appFontScaled(16), CommonStyle::getLinkColor(), Utils::scale_value(44));
        {
            Utils::grabTouchWidget(value);
            value->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
            value->setFading(true);
            value->setText(_value);
            QObject::connect(value, &TextEmojiWidget::clicked, [value, _slot]()
            {
                _slot(value);
            });
            valueLayout->addWidget(value);
        }
        mainLayout->addWidget(valueWidget);

        _layout->addWidget(mainWidget);
        return value;
    }

    GeneralCreator::DropperInfo GeneralCreator::addDropper(QWidget* _parent, QLayout* _layout, const QString& _info, const std::vector< QString >& _values, int _selected, int _width, std::function< void(QString, int, TextEmojiWidget*) > _slot1, std::function< QString(bool) > _slot2)
    {
        TextEmojiWidget* title = nullptr;
        TextEmojiWidget* aw1 = nullptr;
        auto titleWidget = new QWidget(_parent);
        auto titleLayout = Utils::emptyHLayout(titleWidget);
        Utils::grabTouchWidget(titleWidget);
        titleWidget->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        titleLayout->setAlignment(Qt::AlignLeft);
        {
            title = new TextEmojiWidget(titleWidget, Fonts::appFontScaled(16), Ui::CommonStyle::getTextCommonColor(), Utils::scale_value(44));
            Utils::grabTouchWidget(title);
            title->setSizePolicy(QSizePolicy::Policy::Preferred, title->sizePolicy().verticalPolicy());
            title->setText(_info);
            titleLayout->addWidget(title);

            aw1 = new TextEmojiWidget(titleWidget, Fonts::appFontScaled(16), Ui::CommonStyle::getTextCommonColor(), Utils::scale_value(44));
            Utils::grabTouchWidget(aw1);
            aw1->setSizePolicy(QSizePolicy::Policy::Preferred, aw1->sizePolicy().verticalPolicy());
            aw1->setText(" ");
            titleLayout->addWidget(aw1);
        }
        _layout->addWidget(titleWidget);

        auto selectedItemWidget = new QWidget(_parent);
        auto selectedItemLayout = Utils::emptyHLayout(selectedItemWidget);
        Utils::grabTouchWidget(selectedItemWidget);
        selectedItemWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
        selectedItemWidget->setFixedHeight(Utils::scale_value(48));
        selectedItemLayout->setAlignment(Qt::AlignLeft);

        DropperInfo di;
        di.currentSelected = NULL;
        di.menu = NULL;
        {
            auto selectedItemButton = new QPushButton(selectedItemWidget);
            auto dl = Utils::emptyVLayout(selectedItemButton);
            selectedItemButton->setFlat(true);
            selectedItemButton->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
            selectedItemButton->setFixedSize(QSize(Utils::scale_value(_width == -1 ? 280 : _width), Utils::scale_value(48)));
            selectedItemButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
            {
                auto selectedItem = new TextEmojiWidget(selectedItemButton, Fonts::appFontScaled(16), Ui::CommonStyle::getTextCommonColor(), Utils::scale_value(28));
                Utils::grabTouchWidget(selectedItem);

                selectedItem->setText(Utils::getItemSafe(_values, _selected, " "));
                selectedItem->setFading(true);
                dl->addWidget(selectedItem);
                di.currentSelected = selectedItem;

                auto lp = new QWidget(selectedItemButton);
                auto lpl = Utils::emptyHLayout(lp);
                Utils::grabTouchWidget(lp);
                lpl->setAlignment(Qt::AlignBottom);
                {
                    auto ln = new QFrame(lp);
                    Utils::grabTouchWidget(ln);
                    ln->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
                    ln->setFrameShape(QFrame::StyledPanel);
                    Utils::ApplyStyle(ln, DROPDOWN_STYLE);
                    lpl->addWidget(ln);
                }
                dl->addWidget(lp);

                auto menu = new FlatMenu(selectedItemButton);
                for (auto v : _values)
                    menu->addAction(v);
                QObject::connect(menu, &QMenu::triggered, _parent, [menu, aw1, selectedItem, _slot1](QAction* a)
                {
                    int ix = -1;
                    QList<QAction*> allActions = menu->actions();
                    for (QAction* action : allActions)
                    {
                        ix++;
                        if (a == action)
                        {
                            selectedItem->setText(a->text());
                            _slot1(a->text(), ix, aw1);
                            break;
                        }
                    }
                });
                selectedItemButton->setMenu(menu);
                di.menu = menu;
            }
            selectedItemLayout->addWidget(selectedItemButton);
        }
        _layout->addWidget(selectedItemWidget);

        return di;
    }

    void GeneralCreator::addProgresser(QWidget* _parent, QLayout* _layout, const std::vector< QString >& _values, int _selected, std::function< void(TextEmojiWidget*, TextEmojiWidget*, int) > _slot)
    {
        TextEmojiWidget* w = nullptr;
        TextEmojiWidget* aw = nullptr;
        auto mainWidget = new QWidget(_parent);
        Utils::grabTouchWidget(mainWidget);
        auto mainLayout = Utils::emptyHLayout(mainWidget);
        mainWidget->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        mainLayout->setAlignment(Qt::AlignLeft);
        mainLayout->setSpacing(Utils::scale_value(4));
        {
            w = new TextEmojiWidget(mainWidget, Fonts::appFontScaled(16), Ui::CommonStyle::getTextCommonColor(), Utils::scale_value(44));
            Utils::grabTouchWidget(w);
            w->setSizePolicy(QSizePolicy::Policy::Preferred, w->sizePolicy().verticalPolicy());
            w->setText(" ");
            mainLayout->addWidget(w);

            aw = new TextEmojiWidget(mainWidget, Fonts::appFontScaled(16), Ui::CommonStyle::getTextCommonColor(), Utils::scale_value(44));
            Utils::grabTouchWidget(aw);
            aw->setSizePolicy(QSizePolicy::Policy::Preferred, aw->sizePolicy().verticalPolicy());
            aw->setText(" ");
            mainLayout->addWidget(aw);

            _slot(w, aw, _selected);
        }
        _layout->addWidget(mainWidget);

        auto sliderWidget = new QWidget(_parent);
        Utils::grabTouchWidget(sliderWidget);
        auto sliderLayout = Utils::emptyVLayout(sliderWidget);
        sliderWidget->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sliderWidget->setFixedWidth(Utils::scale_value(280));
        sliderLayout->setAlignment(Qt::AlignBottom);
        sliderLayout->setContentsMargins(0, Utils::scale_value(12), 0, 0);
        {
            auto slider = new SettingsSlider(Qt::Orientation::Horizontal, _parent);
            Utils::grabTouchWidget(slider);
            slider->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
            slider->setFixedWidth(Utils::scale_value(280));
            slider->setFixedHeight(Utils::scale_value(24));
            slider->setMinimum(0);
            slider->setMaximum((int)_values.size() - 1);
            slider->setValue(_selected);
            slider->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
            auto SLIDER_STYLE =
                QString("QSlider::handle:horizontal { margin: %1 0 %2 0; }")
                .arg(Utils::scale_value(-12)).arg(Utils::scale_value(-12));
            slider->setStyleSheet(SLIDER_STYLE);
            QObject::connect(slider, &QSlider::valueChanged, [w, aw, _slot](int v)
            {
                _slot(w, aw, v);
                w->update();
                aw->update();
            });
            sliderLayout->addWidget(slider);
        }
        _layout->addWidget(sliderWidget);
    }

    void GeneralCreator::addBackButton(QWidget* _parent, QLayout* _layout, std::function<void()> _onButtonClick)
    {
        auto backArea = new QWidget(_parent);
        backArea->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));
        auto backLayout = new QVBoxLayout(backArea);
        backLayout->setContentsMargins(Utils::scale_value(24), Utils::scale_value(24), 0, 0);

        auto backButton = new BackButton(backArea);
        backButton->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
        backButton->setFlat(true);
        backButton->setFocusPolicy(Qt::NoFocus);
        backButton->setCursor(Qt::CursorShape::PointingHandCursor);
        backLayout->addWidget(backButton);

        auto verticalSpacer = new QSpacerItem(Utils::scale_value(15), Utils::scale_value(543), QSizePolicy::Minimum, QSizePolicy::Expanding);
        backLayout->addItem(verticalSpacer);

        _layout->addWidget(backArea);
        QObject::connect(backButton, &QPushButton::clicked, _onButtonClick);
    }

    SettingsSlider::SettingsSlider(Qt::Orientation _orientation, QWidget* _parent/* = nullptr*/):
        QSlider(_orientation, _parent)
    {
        //
    }

    SettingsSlider::~SettingsSlider()
    {
        //
    }

    void SettingsSlider::mousePressEvent(QMouseEvent* _event)
    {
        QSlider::mousePressEvent(_event);
#ifndef __APPLE__
        if (_event->button() == Qt::LeftButton)
        {
            if (orientation() == Qt::Vertical)
                setValue(minimum() + ((maximum()-minimum() + 1) * (height()-_event->y())) / height());
            else
                setValue(minimum() + ((maximum()-minimum() + 1) * _event->x()) / width());
            _event->accept();
        }
#endif // __APPLE__
    }

    void SettingsSlider::wheelEvent(QWheelEvent* _e)
    {
        // Disable mouse wheel event for sliders
        if (parent())
        {
            parent()->event(_e);
        }
    }
}
