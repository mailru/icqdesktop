#pragma once

class MacMigrationManager;
class MacProfile;
class MacProfileList;

namespace Ui
{
    class account_widget : public QPushButton
    {
        Q_OBJECT

    private Q_SLOTS:
        void avatar_loaded(QString uid);
        
    Q_SIGNALS:
        
    private:
        const MacProfile& account_;
        virtual void paintEvent(QPaintEvent* _e) override;
    public:
        account_widget(QWidget* _parent, const MacProfile &profile);
        virtual ~account_widget();
        
    private:
        QLabel * avatar_;
        int avatar_h_;
    };

    class AccountsPage : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void account_selected();
        void loggedIn();
        
    private Q_SLOTS:
        void loginResult(int);

    private:
        MacMigrationManager *manager_;
        
    public:
        AccountsPage(QWidget* _parent, MacMigrationManager * manager);
        virtual ~AccountsPage();
        
        void summon();
    };

}


