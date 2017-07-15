#pragma once

namespace Ui
{
    class MenuStyle: public QProxyStyle
    {
        Q_OBJECT

    public:
        virtual int pixelMetric(PixelMetric _metric, const QStyleOption* _option = 0, const QWidget* _widget = 0 ) const;

    };

   class ContextMenu : public QMenu
   {
       Q_OBJECT

   public:
       ContextMenu(QWidget* parent);

       static void applyStyle(QMenu* menu, bool withPadding, int fonSize, int height);

       QAction* addActionWithIcon(const QIcon& _icon, const QString& _name, const QVariant& _data);

       QAction* addActionWithIcon(const QIcon& _icon, const QString& _name, const QObject* _receiver, const char* _member);

       QAction* addActionWithIcon(const QString& _iconPath, const QString& _name, const QVariant& _data);

       bool hasAction(const QString& _command);

       void removeAction(const QString& _command);

       void invertRight(bool _invert);
       void setIndent(int _indent);

	   void popup(const QPoint& _pos, QAction* _at=0);
       void clear();

   protected:
       virtual void showEvent(QShowEvent* _e) override;
       virtual void hideEvent(QHideEvent* _e) override;
       virtual void focusOutEvent(QFocusEvent *_e) override;

   private:
       bool InvertRight_;
       int Indent_;
	   QPoint Pos_;
   };
}
