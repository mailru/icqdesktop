#pragma once

#include "../../controls/TextEditEx.h"
#include "../../types/message.h"

class QPushButton;

namespace Ui
{
    class PictureWidget;

    enum DisableReason
    {
        ReadOnlyChat = 0,
        NotAMember = 1,
    };

    class input_edit : public TextEditEx
    {

    public:
        input_edit(QWidget* _parent);
        virtual bool catchEnter(int _modifiers) override;
    };

    class InputWidget : public QWidget
    {
        Q_OBJECT

Q_SIGNALS:

        void smilesMenuSignal();
        void sendMessage(QString);
        void editFocusOut();
        void ctrlFPressedInInputWidget();
        void needUpdateSizes();
        void sizeChanged();

    public Q_SLOTS:
        void quote(QList<Data::Quote>);
        void contactSelected(QString);

    private Q_SLOTS:

        void edit_content_changed();
        void send();
        void attach_file();
        void stats_attach_file();
        void clear_quote(const QString&);
        void clear_files(const QString&);
        void clear();
        void updateSizes();

        void insert_emoji(int32_t _main, int32_t _ext);
        void send_sticker(int32_t _set_id, int32_t _sticker_id);
        void resize_to(int _height);
        
        void typed();

        void stats_message_enter();
        void stats_message_send();

        void onFilesCancel();
        void disableActionClicked(const QString&);
        void chatRoleChanged(QString);

    public:

        InputWidget(QWidget* parent, int height = -1, const QString& styleSheet = QString(), bool messageOnly = false);
        ~InputWidget();
        void hide();
        void hideNoClear();

        void disable(DisableReason reason);
        void enable();

        void predefined(const QString& _contact, const QString& _text);

        Q_PROPERTY(int current_height READ get_current_height WRITE set_current_height)

        void set_current_height(int _val);
        int get_current_height() const;

        void setFocusOnInput();

        void setFontColor(const QColor& _color);

    private:
        void initQuotes(const QString&);
        void initFiles(const QString&);
        QPixmap getFilePreview(const QString& contact);
        QString getFileSendText(const QString& contact);

    private:

        QPushButton* smiles_button_;	
        QPushButton* send_button_;
        QPushButton* file_button_;
        QPropertyAnimation* anim_height_;

        input_edit* text_edit_;
        QString contact_;
        QString predefined_;

        QWidget* quote_text_widget_;
        TextEditEx* quote_text_;
        QWidget* quote_line_;
        QWidget* quote_block_;
        QWidget* files_block_;
        QWidget* input_disabled_block_;
        QPushButton* cancel_quote_;
        QPushButton* cancel_files_;
        QMap<QString, QList<Data::Quote>> quotes_;
        QMap<QString, QStringList> files_to_send_;
        PictureWidget* file_preview_;
        QLabel* files_label_;
        QLabel* disable_label_;
        QMap<QString, QPixmap> image_buffer_;

        int active_height_;
        int need_height_;
        int active_document_height_;
        int default_height_;
        bool is_initializing_;
        bool message_only_;
        QMap<QString, DisableReason>  disabled_;

    protected:

        virtual void paintEvent(QPaintEvent* _e) override;
        virtual void resizeEvent(QResizeEvent * _e) override;
        virtual void keyPressEvent(QKeyEvent * _e) override;
        virtual void keyReleaseEvent(QKeyEvent * _e) override;
    };
}