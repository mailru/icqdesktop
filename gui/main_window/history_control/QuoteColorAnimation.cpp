#include "stdafx.h"

#include "QuoteColorAnimation.h"

QuoteColorAnimation::QuoteColorAnimation(QWidget* parent) 
	: QObject(nullptr)
	, Widget_(parent)
	, Alpha_(255)
    , IsActive_(true)
    , bPlay_(false)
{
}

QColor QuoteColorAnimation::quoteColor() const
{
	return QuoteColor_;
}

void QuoteColorAnimation::setQuoteColor(QColor color)
{
	QuoteColor_ = color;
	Widget_->repaint();
}

void QuoteColorAnimation::setSemiTransparent()
{
	Alpha_ = 105;
}

void QuoteColorAnimation::startQuoteAnimation()
{
    if (!IsActive_)
        return;

    bPlay_ = true;
	/// pause 0.25 sec
	/// color fader green->transparent
	/// pause 0.25 sec
	/// color fader transparent->green
	QPropertyAnimation* delay = new QPropertyAnimation(this, "");
	delay->setDuration(250);
	delay->start();

	connect(delay, &QPropertyAnimation::finished, this, [this]()
	{
		QPropertyAnimation *anim = new QPropertyAnimation(this, "quoteColor");
		anim->setDuration(300);
		anim->setStartValue(QColor(0x84, 0xb8, 0x58, 0));
		anim->setEndValue(QColor(0x84, 0xb8, 0x58, Alpha_));
		anim->start();

		connect(anim, &QPropertyAnimation::finished, this, [this]()
		{
			QPropertyAnimation* delay = new QPropertyAnimation(this, "");
			delay->setDuration(150);
			delay->start();

			connect(delay, &QPropertyAnimation::finished, this, [this]()
			{
				QPropertyAnimation *anim = new QPropertyAnimation(this, "quoteColor");
				anim->setDuration(300);
				anim->setStartValue(QColor(0x84, 0xb8, 0x58, Alpha_));
				anim->setEndValue(QColor(0x84, 0xb8, 0x58, 0));
				anim->start();

				connect(anim, &QPropertyAnimation::finished, this, [this]()
				{
                    bPlay_ = false;
					this->QuoteColor_ = QColor();
					Widget_->repaint();
				});
			});
		});
	});
}

bool QuoteColorAnimation::isPlay() const
{
    return bPlay_;
}

void QuoteColorAnimation::deactivate()
{
    IsActive_ = false;
}
