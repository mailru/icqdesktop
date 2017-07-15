#pragma once

namespace Ui
{
    class LineEditEx;
    class CustomButton;

    class SearchWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void search(QString);
        void searchBegin();
        void searchEnd();
        void inputEmpty();
        void enterPressed();
        void upPressed();
        void downPressed();
        void escapePressed();
        void nonActiveButtonPressed();
        void searchIconClicked();
        void activeChanged(bool _isActive);

    public Q_SLOTS:
        void searchCompleted();

    private Q_SLOTS:
        void searchStarted();
        void searchChanged(QString);
        void editEnterPressed();
        void editUpPressed();
        void editDownPressed();
        void focusedOut();
        void onEscapePress();

    public:
        SearchWidget(QWidget* _parent = 0, int _add_hor_space = 0, int _add_ver_space = 0);
        ~SearchWidget();

        QString getText() const;
        bool isEmpty() const;
        void clearInput();
        void setFocus();
        void clearFocus();

        bool isActive() const { return active_; }

    private:
        void setActive(bool _active);
        LineEditEx* searchEdit_;
        bool active_;

        QVBoxLayout *vMainLayout_;
        QHBoxLayout *hSearchLayout_;
        QHBoxLayout *hMainLayout;
        QWidget *searchWidget_;
        QHBoxLayout *horLineLayout_;
        CustomButton *closeIcon_;
        bool isWithButton_;
    };
}