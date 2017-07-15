#pragma once

class QPushButton;

namespace Themes
{
    class IThemePixmap;

    typedef std::shared_ptr<IThemePixmap> IThemePixmapSptr;
}

namespace Emoji
{
    struct EmojiRecord;
    typedef std::shared_ptr<EmojiRecord> EmojiRecordSptr;
    typedef std::vector<EmojiRecordSptr> EmojiRecordSptrVec;
}

namespace Ui
{
    namespace stickers
    {
        class Set;
        class Sticker;
        typedef std::vector<std::shared_ptr<stickers::Set>> setsArray;
        typedef std::shared_ptr<Set> setSptr;
    }

    class smiles_Widget;

    namespace Smiles
    {
        class TabButton;
        class Toolbar;

        struct emoji_category
        {
            QString									name_;
            const Emoji::EmojiRecordSptrVec&		emojis_;

            emoji_category(const QString& _name, const Emoji::EmojiRecordSptrVec& _emojis)
                :	name_(_name), emojis_(_emojis)
            {
            }
        };

        class EmojiViewItemModel : public QStandardItemModel
        {
            QSize	prevSize_;
            int		emojisCount_;
            int		needHeight_;
            bool	singleLine_;
            bool    snaps_;
            int		spacing_;

            std::vector<emoji_category>		emojiCategories_;

            QVariant data(const QModelIndex& _idx, int _role) const;

        public:

            EmojiViewItemModel(QWidget* _parent, bool _singleLine = false, bool _snaps = false);
            ~EmojiViewItemModel();

            int addCategory(const QString& _category);
            int addCategory(const emoji_category& _category);
            bool resize(const QSize& _size, bool _force = false);
            int getEmojisCount() const;
            int getNeedHeight() const;
            int getCategoryPos(int _index);
            const std::vector<emoji_category>& getCategories() const;
            void onEmojiAdded();
            int spacing() const;
            void setSpacing(int _spacing);

            Emoji::EmojiRecordSptr getEmoji(int _col, int _row) const;

        };

        class EmojiTableItemDelegate : public QItemDelegate
        {
            Q_OBJECT

        public:
            EmojiTableItemDelegate(QObject* parent, bool snaps = false);

            void animate(const QModelIndex& index, int start, int end, int duration);

            void paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const;
            QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const;

            Q_PROPERTY(int prop READ getProp WRITE setProp);

            int getProp() const { return Prop_; }
            void setProp(int val);

        private:
            int Prop_;
            bool Snaps_;
            QPropertyAnimation* Animation_;
            QModelIndex AnimateIndex_;
        };

        class EmojiTableView : public QTableView
        {
            EmojiViewItemModel*		model_;
            EmojiTableItemDelegate* itemDelegate_;

        public:

            EmojiTableView(QWidget* _parent, EmojiViewItemModel* _model);
            ~EmojiTableView();

            virtual void resizeEvent(QResizeEvent * _e);

            int addCategory(const QString& _category);
            int addCategory(const emoji_category& _category);
            int getCategoryPos(int _index);
            const std::vector<emoji_category>& getCategories() const;
            Emoji::EmojiRecordSptr getEmoji(int _col, int _row) const;
            void onEmojiAdded();

        };

        class EmojisWidget : public QWidget
        {
        Q_OBJECT

            EmojiTableView* view_;

            Toolbar* toolbar_;
            std::vector<TabButton*> buttons_;

            void placeToolbar(const QRect& _viewRect);

        Q_SIGNALS:

            void emojiSelected(Emoji::EmojiRecordSptr _emoji);
            void scrollToGroup(const int _pos);

        public:
            void onViewportChanged(const QRect& _rect, bool _blockToolbarSwitch);
            void selectFirstButton();
            EmojisWidget(QWidget* _parent);
            ~EmojisWidget();
        };



        class RecentsStickersTable;

        
        class RecentsWidget : public QWidget
        {
        Q_OBJECT
        Q_SIGNALS:
            void emojiSelected(Emoji::EmojiRecordSptr _emoji);
            void stickerSelected(qint32 _setId, qint32 _stickerId);

        private Q_SLOTS:
            void stickers_event();

        private:
            Emoji::EmojiRecordSptrVec emojis_;

            QVBoxLayout* vLayout_;
            EmojiTableView* emojiView_;
            RecentsStickersTable* stickersView_;

            void init();

        public:

            RecentsWidget(QWidget* _parent);
            ~RecentsWidget();

            void addSticker(int32_t _set_id, int32_t _stickerOd);
            void addEmoji(Emoji::EmojiRecordSptr _emoji);
            void initEmojisFromSettings();
            void initStickersFromSettings();
            void onStickerUpdated(int32_t _setOd, int32_t _stickerOd);
        };




        class StickersTable : public QWidget
        {
            Q_OBJECT
            Q_SIGNALS:

            void stickerSelected(qint32 _setId, qint32 _stickerId);

        protected:
            int32_t stickersSetId_;

            int needHeight_;
            
            QSize prevSize_;
            int columnCount_;
            int rowCount_;

            Themes::IThemePixmapSptr preloader_;

            virtual void resizeEvent(QResizeEvent * _e) override;
            virtual void paintEvent(QPaintEvent* _e) override;
            virtual void mousePressEvent(QMouseEvent* _e) override;

            QRect& getStickerRect(int _index);

            virtual bool resize(const QSize& _size, bool _force = false);
            int getNeedHeight() const;

        public:

            virtual void onStickerUpdated(int32_t _setId, int32_t _stickerId);
            void onStickerAdded();

            StickersTable(QWidget* _parent, int32_t _stickersSetId);
            virtual ~StickersTable();
        };

        typedef std::vector<std::pair<int32_t, int32_t>> recentStickersArray;

        class RecentsStickersTable : public StickersTable
        {
            recentStickersArray recentStickersArray_;

            int maxRowCount_;

            virtual bool resize(const QSize& _size, bool _force = false) override;
            virtual void paintEvent(QPaintEvent* _e) override;
            virtual void mousePressEvent(QMouseEvent* _e) override;

        public:

            bool addSticker(int32_t _setId, int32_t _stickerId);
            virtual void onStickerUpdated(int32_t _setId, int32_t _stickerId) override;

            void setMaxRowCount(int _val);

            const recentStickersArray& getStickers() const;

            RecentsStickersTable(QWidget* _parent);
            virtual ~RecentsStickersTable();
        };



        class StickersWidget : public QWidget
        {
            Q_OBJECT

        Q_SIGNALS:

            void stickerSelected(int32_t _setId, int32_t _stickerId);
            void scrollToSet(int _pos);

        private Q_SLOTS:

        private:

            void insertNextSet(int32_t _setId);

            stickers::setsArray::const_iterator	iterCurrentSet_;

            QVBoxLayout* vLayout_;
            Toolbar* toolbar_;
            std::map<int32_t, StickersTable*> setTables_;

            bool initialized_;

        public:

            void init();
            void onStickerUpdated(int32_t _setId, int32_t _stickerId);

            StickersWidget(QWidget* _parent, Toolbar* _toolbar);
            ~StickersWidget();
        };


        class SmilesMenu : public QFrame
        {
            Q_OBJECT
            Q_SIGNALS:

            void emojiSelected(int32_t _main, int32_t _ext);
            void stickerSelected(int32_t _setId, int32_t _stickerId);

        private Q_SLOTS:

                void im_created();
                void touchScrollStateChanged(QScroller::State);
                void stickersMetaEvent();
                void stickerEvent(qint32 _setId, qint32 _stickerId);

        public:

            SmilesMenu(QWidget* _parent);
            ~SmilesMenu();

            void Show();
            void Hide();
            void ShowHide();
            bool IsHidden() const;

            Q_PROPERTY(int currentHeight READ getCurrentHeight WRITE setCurrentHeight)

            void setCurrentHeight(int _val);
            int getCurrentHeight() const;

        private:

            Toolbar* topToolbar_;
            Toolbar* bottomToolbar_;

            QScrollArea* viewArea_;

            RecentsWidget* recentsView_;
            EmojisWidget* emojiView_;
            StickersWidget* stickersView_;
            QVBoxLayout* rootVerticalLayout_;
            QPropertyAnimation* animHeight_;
            QPropertyAnimation* animScroll_;

            bool isVisible_;
            bool blockToolbarSwitch_;

            bool stickerMetaRequested_;

            void InitSelector();
            void InitStickers();
            void InitResents();

            void ScrollTo(int _pos);
            void HookScroll();

        protected:

            int currentHeight_;

            virtual void paintEvent(QPaintEvent* _e) override;
            virtual void focusOutEvent(QFocusEvent* _event) override;
            virtual void focusInEvent(QFocusEvent* _event) override;
            virtual void resizeEvent(QResizeEvent * _e) override;

        };
    }
}