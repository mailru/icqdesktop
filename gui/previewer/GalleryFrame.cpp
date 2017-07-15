#include "stdafx.h"

#include "../controls/CustomButton.h"
#include "../utils/utils.h"

#include "GalleryFrame.h"

namespace
{
    const int buttonSize = 40;
    const QString imagePath1 = ":/resources/previewer/viewer_%1_100.png";
    const QString imagePath2 = ":/resources/previewer/viewer_%1_100_%2.png";
}

Previewer::GalleryFrame::GalleryFrame(QWidget* _parent)
    : QFrame(_parent)
{
    const auto style = Utils::LoadStyle(":/resources/previewer/qss/frame.qss");
    setStyleSheet(style);
    setProperty("GalleryFrame", true);

    layout_ = Utils::emptyHLayout();
    setLayout(layout_);
}

Ui::CustomButton* Previewer::GalleryFrame::addButton(const QString& name, ButtonStates allowableStates)
{
    auto button = new Ui::CustomButton(this, getImagePath(name, Default));

    if (allowableStates & Disabled)
        button->setDisabledImage(getImagePath(name, Disabled));
    if (allowableStates & Hover)
        button->setHoverImage(getImagePath(name, Hover));
    if (allowableStates & Pressed)
        button->setPressedImage(getImagePath(name, Pressed));

    const auto size = Utils::scale_value(buttonSize);

    button->setFixedSize(size, size);

    button->setCursor(Qt::PointingHandCursor);

    layout_->addWidget(button);

    return button;
}

QLabel* Previewer::GalleryFrame::addLabel()
{
    const auto style = Utils::LoadStyle(":/resources/previewer/qss/label.qss");

    auto label = new QLabel(this);
    label->setStyleSheet(style);
    label->setProperty("GalleryLabel", true);

    layout_->addWidget(label);
    return label;
}

void Previewer::GalleryFrame::addSpace(SpaceSize _value)
{
    layout_->addSpacing(Utils::scale_value(_value));
}

void Previewer::GalleryFrame::mousePressEvent(QMouseEvent* _event)
{
    _event->accept();
}

QString Previewer::GalleryFrame::getImagePath(const QString& name, ButtonStates state) const
{
    switch (state)
    {
        case Default:   return imagePath1.arg(name);
        case Disabled:  return imagePath2.arg(name).arg("disable");
        case Hover:     return imagePath2.arg(name).arg("hover");
        case Pressed:   return imagePath2.arg(name).arg("active");
    }
    assert(!"invalid kind!");
    return QString();
}
