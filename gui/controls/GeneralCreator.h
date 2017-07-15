#pragma once

namespace Ui
{
    class TextEmojiWidget;
        
    struct Synchronizator
    {
        std::vector<QWidget *> widgets_;
        const char *signal_;
        const char *slot_;
        Synchronizator(QWidget *widget, const char *signal, const char *slot): signal_(signal), slot_(slot)
        {
            widgets_.push_back(widget);
        }
    };
    
    class SettingsSlider: public QSlider
    {
    private:
        void mousePressEvent(QMouseEvent* _event) override;
        void wheelEvent(QWheelEvent* _e) override;

    public:
        explicit SettingsSlider(Qt::Orientation _orientation, QWidget* _parent = nullptr);
        ~SettingsSlider();
    };

    struct GeneralCreator
    {
        struct addSwitcherWidgets
        {
            addSwitcherWidgets() 
                : text_(0)
                , check_(0)
            {

            }

            TextEmojiWidget *text_;
            QCheckBox *check_;
        };

        struct DropperInfo
        {
            QMenu* menu;
            TextEmojiWidget* currentSelected;
        };

        static void addHeader(
                              QWidget* _parent,
                              QLayout* _layout,
                              const QString& _text
                              );
        
        static GeneralCreator::addSwitcherWidgets addSwitcher(
                                                              std::map<std::string, Synchronizator>* _collector,
                                                              QWidget* _parent,
                                                              QLayout* _layout,
                                                              const QString& _text,
                                                              bool _switched,
                                                              std::function< QString(bool) > _slot
                                                              );
        
        static TextEmojiWidget* addChooser(
                                           QWidget* _parent,
                                           QLayout* _layout,
                                           const QString& _info,
                                           const QString& _value,
                                           std::function< void(TextEmojiWidget*) > _slot
                                           );
        
        static DropperInfo addDropper(
                                      QWidget* _parent,
                                      QLayout* _layout,
                                      const QString& _info,
                                      const std::vector< QString >& _values,
                                      int _selected,
                                      int _width,
                                      std::function< void(QString, int, TextEmojiWidget*) > _slot1,
                                      std::function< QString(bool) > _slot2
                                      );
        
        static void addProgresser(
                                  QWidget* _parent,
                                  QLayout* _layout,
                                  const std::vector< QString >& _values,
                                  int _selected,
                                  std::function< void(TextEmojiWidget*, TextEmojiWidget*, int) > _slot
                                  );
        
        static void addBackButton(
                                  QWidget* _parent,
                                  QLayout* _layout,
                                  std::function<void()> _on_button_click = [](){}
                                  );
        
    };
}
