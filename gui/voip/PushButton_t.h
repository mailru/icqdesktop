#ifndef __PUSH_BUTTON_T_H__
#define __PUSH_BUTTON_T_H__

namespace Ui
{
    class CompiledText;

    class PushButton_t : public QPushButton {
    public:
        enum eButtonState {
            normal = 0, 
            hovered, 
            pressed, 
            //--------
            total
        };

    public:
        PushButton_t(QWidget* parent = NULL);
        void setImageForState(const eButtonState state, const std::string& image);
        void setColorForState(const eButtonState state, const QColor& color);
        void setOffsets(int fromIconToText);
        void setIconSize(const int w, const int h);
        void setAlignment(Qt::Alignment alignment);
        void setText(const QString& prefix, const QString& postfix);
        void setPrefix(const QString& prefix);

        void setPostfix(const QString& postfix);
        void setPostfixColor(const QColor& color);

        int precalculateWidth();
        void setState(const eButtonState state);

    protected:
        void paintEvent(QPaintEvent*) override;
        bool event(QEvent*) override;

        void setPrefixInternal(const QString& prefix);

    private:
        eButtonState  currentState_;
        QPixmap       bitmapsForStates_[total];
        QColor        colorsForStates_[total];
        int           fromIconToText_;
        int           iconW_;
        int           iconH_;
        Qt::Alignment alignment_;
        QString       postfix_;
        QColor        postfixColor_;
        std::unique_ptr<CompiledText>	compiledText_;
    };   
}

#endif//__PUSH_BUTTON_T_H__
