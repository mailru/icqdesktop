#ifndef __NAME_AND_STATUS_WIDGET_H__
#define __NAME_AND_STATUS_WIDGET_H__
#include "../controls/TextEmojiWidget.h"

namespace Ui
{

    template<typename __Base>
    class ShadowedWidget : public __Base
    {
    public:
        ShadowedWidget(QWidget* _parent = NULL, int _tailLen = 30, double _alphaFrom = 1, double _alphaTo = 0);
        virtual ~ShadowedWidget();

    protected:
        void paintEvent(QPaintEvent*);
        void resizeEvent(QResizeEvent*);

    private:
        QLinearGradient linearGradient_;
        int tailLenPx_;
    };

    class NameAndStatusWidget : public QWidget
    {
        Q_OBJECT
    public:
        NameAndStatusWidget(QWidget* _parent, int _nameBaseline, int _statusBaseline);
        virtual ~NameAndStatusWidget();

        void setName  (const char* _name);
        void setStatus(const char* _status);

        void setNameProperty(const char* _propName, bool _val);
        void setStatusProperty(const char* _propName, bool _val);

    private:
        TextEmojiLabel* name_;
        TextEmojiLabel* status_;
    };
    
    class NameWidget : public QWidget
    {
        Q_OBJECT
    public:
        NameWidget(QWidget* _parent, int _nameBaseline);
        virtual ~NameWidget();
        
        void setName  (const char* _name);
        
        void setNameProperty(const char* _propName, bool _val);
        
    private:
        TextEmojiLabel* name_;
    };
}

#endif//__NAME_AND_STATUS_WIDGET_H__