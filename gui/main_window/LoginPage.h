#pragma once

#include "../types/common_phone.h"

namespace core
{
    namespace stats
    {
        enum class stats_event_names;
    }
}

namespace Ui
{
	class LineEditEx;
    class TextEditEx;
	class CountrySearchCombobox;
    class CustomButton;
    class BackButton;
    
	class LoginPage : public QWidget
	{
		Q_OBJECT
	Q_SIGNALS:
		void country(QString);
		void loggedIn();
        void attached();

	private Q_SLOTS:
		void countrySelected(QString);
		void nextPage();
		void updateTimer();
		void sendCode();
		void setPhoneFocusIn();
		void setPhoneFocusOut();
		void getSmsResult(int64_t, int, int);
		void loginResult(int64_t, int);
		void loginResultAttachUin(int64_t, int);
		void loginResultAttachPhone(int64_t, int);
		void clearErrors(bool ignorePhoneInfo = false);
		void codeEditChanged(QString);
		void countryCodeChanged(QString);
		void redrawCountryCode();
		void emptyPhoneRemove();
        void stats_edit_phone();
        void stats_resend_sms();
        void phoneInfoResult(qint64, Data::PhoneInfo);
        void phoneTextChanged();
        void authError(const int _result);

    public Q_SLOTS:
		void prevPage();
		void switchLoginType();
		void updateFocus();
        void openProxySettings();

	public:
		LoginPage(QWidget* _parent, bool _isLogin);
		~LoginPage();

	protected:
		virtual void keyPressEvent(QKeyEvent* _event) override;
		virtual void paintEvent(QPaintEvent* _event) override;
        virtual void showEvent(QShowEvent *_event) override;

	private:
		void init();
		void initLoginSubPage(int _index);
		void setErrorText(int _result);
        void setErrorText(const QString& _customError);
        void updateErrors(int _result);

	private:
		QTimer*						timer_;
		LineEditEx*					countryCode_;
		LineEditEx*					phone_;
		CountrySearchCombobox*		combobox_;
		unsigned int				remainingSeconds_;
		QString						prevCountryCode_;

		QStackedWidget*				loginStakedWidget;
		QPushButton*				nextButton;
		QWidget*					countrySearchWidget;
        BackButton*                 backButton;
        QPushButton*                proxySettingsButton;
		QPushButton*				editPhoneButton;
		QPushButton*				switchButton;
		QPushButton*				resendButton;
		QFrame*						phoneWidget;
		LineEditEx*					uinEdit;
		LineEditEx*					passwordEdit;
        QCheckBox*                  keepLogged;
        LineEditEx*					codeEdit;
		QLabel*						errorLabel;
		QLabel*						enteredPhone;
        QLabel*						hintLabel;
        QPushButton*				passwordForgottenLabel;
        bool                        isLogin_;
        int64_t                     sendSeq_;
        int                         codeLength_;
        bool                        phoneChangedAuto_;
        
        Data::PhoneInfo             receivedPhoneInfo_;
	};
}
