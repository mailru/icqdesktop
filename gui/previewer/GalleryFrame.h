#pragma once

class QHBoxLayout;
class QLabel;
class QPushButton;

namespace Ui
{
    class CustomButton;
}

namespace Previewer
{
    class GalleryFrame
        : public QFrame
    {
        Q_OBJECT
    public:
        explicit GalleryFrame(QWidget* _parent);

        enum ButtonState
        {
            Default     = 0x1,
            Disabled    = 0x4,
            Hover       = 0x10,
            Pressed     = 0x20
        };

        Q_DECLARE_FLAGS(ButtonStates, ButtonState)

        Ui::CustomButton* addButton(const QString& name, ButtonStates allowableStates);

        QLabel* addLabel();

        enum SpaceSize : int
        {
            Small = 8,
            Medium = 12,
            Large = 24
        };

        void addSpace(SpaceSize _value);

    protected:
        void mousePressEvent(QMouseEvent* _event) override;

    private:
        QString getImagePath(const QString& name, ButtonStates state) const;

    private:
        QHBoxLayout* layout_;
    };
}

Q_DECLARE_OPERATORS_FOR_FLAGS(Previewer::GalleryFrame::ButtonStates)
