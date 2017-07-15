#pragma once
namespace Logic
{
    class ChatMembersModel;
	class AbstractSearchModel;
}

namespace Ui
{
    class SearchWidget;
    class ContactList;
    class GeneralDialog;
    class qt_gui_settings;
    class TextEmojiLabel;

    const int defaultInvalidCoord = -1;

    class SelectContactsWidget : public QDialog
    {
        Q_OBJECT

    private Q_SLOTS:
        void enterPressed();
        void itemClicked(const QString&);
        void searchBegin();
        void searchEnd();
        void onViewAllMembers();
        void finished();
        void escapePressed();
        void searchWidgetEnd();

    public Q_SLOTS:
        void UpdateMembers();
        void UpdateViewForIgnoreList(bool _isEmptyIgnoreList);
        void UpdateView();
        void UpdateContactList();

        void reject() override;

    public:
        SelectContactsWidget(const QString& _labelText, QWidget* _parent);

        SelectContactsWidget(
            Logic::ChatMembersModel* _chatMembersModel,
            int _regim,
            const QString& _labelText,
            const QString& _buttonText,
            const QString& _bottomText,
            QWidget* _parent,
            bool _handleKeyPressEvents = true,
			Logic::AbstractSearchModel* searchModel = nullptr);

        ~SelectContactsWidget();

        bool show();
        bool show(int _x, int _y);

        void setView(bool _isShortView);

        const QString& getSelectedContact() const;
        void setSort(bool _isClSorting);

        // Set maximum restriction for selected item count. Used for video conference.
        virtual void setMaximumSelectedCount(int number);

        
    protected:
        void init(const QString& _labelText, const QString& _buttonText = QString());

        QRect CalcSizes() const;
        bool isCheckboxesVisible() const;
        bool isShareLinkMode() const;
        bool isShareTextMode() const;
        bool isVideoConference() const;

        bool forwardConfirmed(QString aimId);
        
        SearchWidget*                            searchWidget_;
        ContactList*                             contactList_;
        std::unique_ptr<GeneralDialog>           mainDialog_;
        QVBoxLayout*                             globalLayout_;

        double                                   koeffWidth_;
        int                                      regim_;
        Logic::ChatMembersModel*                 chatMembersModel_;
        int                                      x_;
        int                                      y_;
        bool                                     isShortView_;
        QWidget*                                 mainWidget_;
        QString                                  bottomText_;

        QString                                  selectedContact_;
        bool                                     sortCL_;
        
        bool                                     handleKeyPressEvents_;

        int                                      maximumSelectedCount_;
		Logic::AbstractSearchModel*				 searchModel_;
    };
}
