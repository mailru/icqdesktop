#include "stdafx.h"
#include "SmilesMenu.h"

#include "toolbar.h"
#include "../ContactDialog.h"
#include "../input_widget/InputWidget.h"
#include "../MainWindow.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../cache/emoji/Emoji.h"
#include "../../cache/emoji/EmojiDb.h"
#include "../../cache/stickers/stickers.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../themes/ResourceIds.h"
#include "../../themes/ThemePixmap.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"

namespace Ui
{
    const int32_t max_stickers_count = 20;

    const int SNAPS_EMOJI_SIZE = 26;

    namespace
    {
        qint32 getEmojiItemSize(bool);

        qint32 getStickerItemSize();

        qint32 getStickerSize();
    }

    using namespace Smiles;

    Emoji::EmojiSizePx getPickerEmojiSize()
    {
        Emoji::EmojiSizePx emojiSize = Emoji::EmojiSizePx::_32;
        int scale = (int) (Utils::getScaleCoefficient() * 100.0);
        scale = Utils::scale_bitmap(scale);
        switch (scale)
        {
        case 100:
            emojiSize = Emoji::EmojiSizePx::_32;
            break;
        case 125:
            emojiSize = Emoji::EmojiSizePx::_40;
            break;
        case 150:
            emojiSize = Emoji::EmojiSizePx::_48;
            break;
        case 200:
            emojiSize = Emoji::EmojiSizePx::_64;
            break;
        default:
            assert(!"invalid scale");
        }

        return emojiSize;
    }

    Emoji::EmojiSizePx getSnapsEmojiSize()
    {
        Emoji::EmojiSizePx emojiSize = Emoji::EmojiSizePx::_27;
        int scale = (int) (Utils::getScaleCoefficient() * 100.0);
        scale = Utils::scale_bitmap(scale);
        switch (scale)
        {
        case 100:
            emojiSize = Emoji::EmojiSizePx::_27;
            break;
        case 125:
            emojiSize = Emoji::EmojiSizePx::_32;
            break;
        case 150:
            emojiSize = Emoji::EmojiSizePx::_40;
            break;
        case 200:
            emojiSize = Emoji::EmojiSizePx::_64;
            break;
        default:
            assert(!"invalid scale");
        }

        return emojiSize;
    }


    //////////////////////////////////////////////////////////////////////////
    // class ViewItemModel
    //////////////////////////////////////////////////////////////////////////
    EmojiViewItemModel::EmojiViewItemModel(QWidget* _parent, bool _singleLine, bool _snaps)
        :	QStandardItemModel(_parent),
        emojisCount_(0),
        needHeight_(0),
        singleLine_(_singleLine),
        spacing_(_snaps ? 4 : 12),
        snaps_(_snaps)
    {
        emojiCategories_.reserve(10);
    }

    EmojiViewItemModel::~EmojiViewItemModel()
    {
    }

    int EmojiViewItemModel::spacing() const
    {
        return spacing_;
    }

    void EmojiViewItemModel::setSpacing(int _spacing)
    {
        spacing_ = _spacing;
    }

    Emoji::EmojiRecordSptr EmojiViewItemModel::getEmoji(int _col, int _row) const
    {
        int index = _row * columnCount() + _col;

        if (index < getEmojisCount())
        {
            int count = 0;
            for (const auto& category : emojiCategories_)
            {
                if ((count + (int) category.emojis_.size()) > index)
                {
                    int indexInCategory = index - count;

                    assert(indexInCategory >= 0);
                    assert(indexInCategory < (int) category.emojis_.size());

                    return category.emojis_[indexInCategory];
                }

                count += category.emojis_.size();
            }
        }

        assert(!"invalid emoji number");

        return nullptr;
    }

    QVariant EmojiViewItemModel::data(const QModelIndex& _idx, int _role) const
    {
        if (_role == Qt::DecorationRole)
        {
            int index = _idx.row() * columnCount() + _idx.column();
            if (index < getEmojisCount())
            {
                auto emoji = getEmoji(_idx.column(), _idx.row());
                if (emoji)
                {
                    auto emoji_ = Emoji::GetEmoji(emoji->Codepoint_, emoji->ExtendedCodepoint_, snaps_ ? getSnapsEmojiSize() : getPickerEmojiSize());
                    QPixmap emojiPixmap = QPixmap::fromImage(emoji_);
                    if (snaps_)
                        emojiPixmap = emojiPixmap.scaled(QSize(Utils::scale_value(SNAPS_EMOJI_SIZE), Utils::scale_value(SNAPS_EMOJI_SIZE)), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    Utils::check_pixel_ratio(emojiPixmap);
                    return emojiPixmap;
                }
            }
        }

        return QVariant();
    }

    int EmojiViewItemModel::addCategory(const QString& _category)
    {
        const Emoji::EmojiRecordSptrVec& emojisVector = Emoji::GetEmojiInfoByCategory(_category);
        emojiCategories_.emplace_back(_category, emojisVector);

        emojisCount_ += emojisVector.size();

        resize(prevSize_);

        return ((int)emojiCategories_.size() - 1);
    }

    int EmojiViewItemModel::addCategory(const emoji_category& _category)
    {
        emojiCategories_.push_back(_category);

        emojisCount_ += _category.emojis_.size();

        return ((int)emojiCategories_.size() - 1);
    }

    int EmojiViewItemModel::getEmojisCount() const
    {
        return emojisCount_;
    }

    int EmojiViewItemModel::getNeedHeight() const
    {
        return needHeight_;
    }

    int EmojiViewItemModel::getCategoryPos(int _index)
    {
        const int columnWidth = getEmojiItemSize(snaps_);
        int columnCount = prevSize_.width() / columnWidth;

        int emojiCountBefore = 0;

        for (int i = 0; i < _index; i++)
            emojiCountBefore += emojiCategories_[i].emojis_.size();

        if (columnCount == 0)
            return 0;

        int rowCount = (emojiCountBefore / columnCount) + (((emojiCountBefore % columnCount) > 0) ? 1 : 0);

        return (((rowCount == 0) ? 0 : (rowCount - 1)) * getEmojiItemSize(snaps_));
    }

    const std::vector<emoji_category>& EmojiViewItemModel::getCategories() const
    {
        return emojiCategories_;
    }

    bool EmojiViewItemModel::resize(const QSize& _size, bool _force)
    {
        const int columnWidth = getEmojiItemSize(snaps_);
        int emojiCount = getEmojisCount();

        bool resized = false;

        if ((prevSize_.width() != _size.width() && _size.width() > columnWidth) || _force)
        {
            int columnCount = _size.width()/ columnWidth;
            if (columnCount > emojiCount)
                columnCount = emojiCount;

            int rowCount  = 0;
            if (columnCount > 0)
            {
                rowCount = (emojiCount / columnCount) + (((emojiCount % columnCount) > 0) ? 1 : 0);
                if (singleLine_ && rowCount > 1)
                    rowCount = 1;
            }

            setColumnCount(columnCount);
            setRowCount(rowCount);

            needHeight_ = getEmojiItemSize(snaps_) * rowCount;

            resized = true;
        }

        prevSize_ = _size;

        return resized;
    }

    void EmojiViewItemModel::onEmojiAdded()
    {
        emojisCount_ = 0;
        for (uint32_t i = 0; i < emojiCategories_.size(); i++)
            emojisCount_ += emojiCategories_[i].emojis_.size();
    }

    //////////////////////////////////////////////////////////////////////////
    // TableView class
    //////////////////////////////////////////////////////////////////////////
    EmojiTableView::EmojiTableView(QWidget* _parent, EmojiViewItemModel* _model)
        :	QTableView(_parent), model_(_model), itemDelegate_(new EmojiTableItemDelegate(this))
    {
        setModel(model_);
        setItemDelegate(itemDelegate_);
        setShowGrid(false);
        verticalHeader()->hide();
        horizontalHeader()->hide();
        setEditTriggers(QAbstractItemView::NoEditTriggers);
        verticalHeader()->setDefaultSectionSize(getEmojiItemSize(false));
        horizontalHeader()->setDefaultSectionSize(getEmojiItemSize(false));
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setFocusPolicy(Qt::NoFocus);
        setSelectionMode(QAbstractItemView::NoSelection);
        setCursor(QCursor(Qt::PointingHandCursor));
    }

    EmojiTableView::~EmojiTableView()
    {
    }

    void EmojiTableView::resizeEvent(QResizeEvent * _e)
    {
        if (model_->resize(_e->size()))
            setFixedHeight(model_->getNeedHeight());

        QTableView::resizeEvent(_e);
    }

    int EmojiTableView::addCategory(const QString& _category)
    {
        return model_->addCategory(_category);
    }

    int EmojiTableView::addCategory(const emoji_category& _category)
    {
        return model_->addCategory(_category);
    }

    int EmojiTableView::getCategoryPos(int _index)
    {
        return model_->getCategoryPos(_index);
    }

    const std::vector<emoji_category>& EmojiTableView::getCategories() const
    {
        return model_->getCategories();
    }

    Emoji::EmojiRecordSptr EmojiTableView::getEmoji(int _col, int _row) const
    {
        return model_->getEmoji(_col, _row);
    }

    void EmojiTableView::onEmojiAdded()
    {
        model_->onEmojiAdded();

        QRect rect = geometry();

        if (model_->resize(QSize(rect.width(), rect.height()), true))
            setFixedHeight(model_->getNeedHeight());
    }

    //////////////////////////////////////////////////////////////////////////
    // EmojiTableItemDelegate
    //////////////////////////////////////////////////////////////////////////
    EmojiTableItemDelegate::EmojiTableItemDelegate(QObject* parent, bool snaps)
        : QItemDelegate(parent)
        , Prop_(0)
        , Snaps_(snaps)
    {
        Animation_ = new QPropertyAnimation(this, "prop");
    }

    void EmojiTableItemDelegate::animate(const QModelIndex& index, int start, int end, int duration)
    {
        AnimateIndex_ = index;
        Animation_->setStartValue(start);
        Animation_->setEndValue(end);
        Animation_->setDuration(duration);
        Animation_->start();
    }

    void EmojiTableItemDelegate::paint(QPainter* _painter, const QStyleOptionViewItem&, const QModelIndex& _index) const
    {
        const EmojiViewItemModel *itemModel = (EmojiViewItemModel *)_index.model();
        QPixmap data = _index.data(Qt::DecorationRole).value<QPixmap>();
        int col = _index.column();
        int row = _index.row();
        int spacing = itemModel->spacing();
        int size = Snaps_ ? Utils::scale_value(SNAPS_EMOJI_SIZE) : (int)getPickerEmojiSize() / Utils::scale_bitmap(1);
        int smileSize = size;
        int addSize = 0;
        if (AnimateIndex_ == _index)
        {
            size = size * Prop_ / Animation_->endValue().toFloat();
            addSize = (smileSize - size) / 2;
        }

        _painter->drawPixmap(col * (smileSize + Utils::scale_value(spacing)) + addSize, row * (smileSize + Utils::scale_value(spacing)) + addSize, size, size, data);
    }

    QSize EmojiTableItemDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const
    {
        int size = Snaps_ ? Utils::scale_value(SNAPS_EMOJI_SIZE) : (int)getPickerEmojiSize() / Utils::scale_bitmap(1);
        return QSize(size, size);
    }

    void EmojiTableItemDelegate::setProp(int val)
    {
        Prop_ = val;
        if (AnimateIndex_.isValid())
            emit ((QAbstractItemModel*)AnimateIndex_.model())->dataChanged(AnimateIndex_, AnimateIndex_);
    }

    //////////////////////////////////////////////////////////////////////////
    // EmojiViewWidget
    //////////////////////////////////////////////////////////////////////////
    EmojisWidget::EmojisWidget(QWidget* _parent)
        :	QWidget(_parent)
    {
        QVBoxLayout* vLayout = Utils::emptyVLayout();
        setLayout(vLayout);

        QLabel* setHeader = new QLabel(this);
        setHeader->setObjectName("set_header");
        setHeader->setText(QT_TRANSLATE_NOOP("input_widget", "EMOJI"));
        vLayout->addWidget(setHeader);

        view_ = new EmojiTableView(this, new EmojiViewItemModel(this));
        view_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        Utils::grabTouchWidget(view_);

        toolbar_ = new Toolbar(this, buttons_align::center);
        toolbar_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        toolbar_->setFixedHeight(Utils::scale_value(48));
        toolbar_->setObjectName("smiles_cat_selector");

        TabButton* firstButton = nullptr;

        QStringList emojiCategories = Emoji::GetEmojiCategories();

        buttons_.reserve(emojiCategories.size());

        for (auto category : emojiCategories)
        {
            QString resourceString = QString(":/resources/smiles_menu/picker_emojitab_") + category + "_100.png";
            TabButton* button = toolbar_->addButton(resourceString);
            button->setProperty("underline", false);
            buttons_.push_back(button);

            int categoryIndex = view_->addCategory(category);

            if (!firstButton)
                firstButton = button;

            connect(button, &TabButton::clicked, [this, categoryIndex]()
            {
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::picker_cathegory_click);
                emit scrollToGroup(view_->geometry().top() + view_->getCategoryPos(categoryIndex));
            });
        }

        vLayout->addWidget(view_);

        QWidget* toolbarPlace = new QWidget(this);
        toolbarPlace->setObjectName("red_marker");
        toolbarPlace->setFixedHeight(toolbar_->height());
        vLayout->addWidget(toolbarPlace);

        if (firstButton)
            firstButton->toggle();

        connect(view_, &EmojiTableView::clicked, [this](const QModelIndex & _index)
        {
            auto emoji = view_->getEmoji(_index.column(), _index.row());
            if (emoji)
            {
                emit emojiSelected(emoji);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::smile_sent_picker);
            }
        });

        toolbar_->raise();
    }

    EmojisWidget::~EmojisWidget()
    {
    }

    void EmojisWidget::onViewportChanged(const QRect& _viewRect, bool _blockToolbarSwitch)
    {
        if (!_blockToolbarSwitch)
        {
            auto categories = view_->getCategories();
            for (uint32_t i = 0; i < categories.size(); i++)
            {
                int pos = view_->getCategoryPos(i);

                if (_viewRect.top() < pos && pos < (_viewRect.top() + _viewRect.width()/2))
                {
                    buttons_[i]->setChecked(true);
                    break;
                }
            }
        }

        placeToolbar(_viewRect);
    }

    void EmojisWidget::placeToolbar(const QRect& _viewRect)
    {
        int h = toolbar_->height();

        QRect thisRect = rect();

        int y = _viewRect.bottom() - h;
        if (_viewRect.bottom() > thisRect.bottom() || ((-1 *(_viewRect.top()) > (_viewRect.height() / 2))))
            y = thisRect.bottom() - h;

        toolbar_->setGeometry(0, y, thisRect.width(), h);
    }

    void EmojisWidget::selectFirstButton()
    {
        if (buttons_.size() > 0)
        {
            buttons_[0]->setChecked(true);
        }
    }









    StickersTable::StickersTable(QWidget* _parent, int32_t _stickersSetId)
        :	QWidget(_parent),
        needHeight_(0),
        stickersSetId_(_stickersSetId),
        columnCount_(0),
        rowCount_(0)
    {
        setCursor(QCursor(Qt::PointingHandCursor));

        preloader_ = Themes::GetPixmap(Themes::PixmapResourceId::StickerPickerPlaceholder);
        assert(preloader_);
    }

    StickersTable::~StickersTable()
    {
    }

    void StickersTable::resizeEvent(QResizeEvent * _e)
    {
        if (resize(_e->size()))
            setFixedHeight(getNeedHeight());

        QWidget::resizeEvent(_e);
    }

    void StickersTable::onStickerAdded()
    {
        QRect rect = geometry();

        if (resize(QSize(rect.width(), rect.height()), true))
            setFixedHeight(getNeedHeight());
    }

    bool StickersTable::resize(const QSize& _size, bool _force)
    {
        const int columnWidth = getStickerItemSize();
        
        int stickersCount = (int) Stickers::getSetStickersCount(stickersSetId_);

        bool resized = false;

        if ((prevSize_.width() != _size.width() && _size.width() > columnWidth) || _force)
        {
            columnCount_ = _size.width()/ columnWidth;
            if (columnCount_ > stickersCount)
                columnCount_ = stickersCount;

            rowCount_ = 0;
            if (stickersCount > 0 && columnCount_ > 0)
            {
                rowCount_ = (stickersCount / columnCount_) + (((stickersCount % columnCount_) > 0) ? 1 : 0);
            }

            needHeight_ = getStickerItemSize() * rowCount_;

            resized = true;
        }

        prevSize_ = _size;

        return resized;
    }

    int StickersTable::getNeedHeight() const
    {
        return needHeight_;
    }

    void StickersTable::onStickerUpdated(int32_t _setId, int32_t _stickerId)
    {
        auto stickerPosInSet = Stickers::getStickerPosInSet(_setId, _stickerId);
        if (stickerPosInSet >= 0)
            update(getStickerRect(stickerPosInSet));
    }

    QRect& StickersTable::getStickerRect(int _index)
    {
        static QRect rect;

        if (columnCount_ > 0 && rowCount_ > 0)
        {
            int itemY = _index / columnCount_;
            int itemX = _index % columnCount_;

            int itemSize = getStickerItemSize();
            int stickSize = getStickerSize();

            int x = itemSize * itemX;
            int y = itemSize * itemY;

            int space = ((getStickerItemSize() - getStickerSize()) / 2);

            rect.setRect(x + space, y + space, stickSize, stickSize);
        }
        else
        {
            rect.setRect(0, 0, 0, 0);
        }

        return rect;
    }

    void StickersTable::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);

        const auto& stickersList = Stickers::getStickers(stickersSetId_);

        for (uint32_t i = 0; i < stickersList.size(); ++i)
        {
            const auto& stickerRect = getStickerRect(i);
            if (!_e->rect().intersects(stickerRect))
            {
                continue;
            }

            auto image = Stickers::getStickerImage(stickersSetId_, stickersList[i], core::sticker_size::small);
            if (image.isNull())
            {
                assert(preloader_);
                preloader_->Draw(p, stickerRect);

                continue;
            }

            if (platform::is_apple() && Utils::is_mac_retina())
            {
                QSize newSize = image.size() / 2;

                if (newSize.width() > stickerRect.size().width() ||
                    newSize.height() > stickerRect.size().height())
                {
                    newSize = image.size().scaled(stickerRect.size(), Qt::KeepAspectRatio);
                }

                QRect targetRect(
                    stickerRect.left(),
                    stickerRect.top(),
                    newSize.width(),
                    newSize.height()
                );

                image = image.scaled(newSize.width() * 2, newSize.height() * 2, Qt::KeepAspectRatio, Qt::SmoothTransformation);

                Utils::check_pixel_ratio(image);

                targetRect.moveCenter(stickerRect.center());

                p.drawImage(targetRect, image, image.rect());
            }
            else
            {
                auto pixmap = QPixmap::fromImage(image);
                pixmap = pixmap.scaled(stickerRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

                QRect targetRect(
                    stickerRect.left(),
                    stickerRect.top(),
                    pixmap.width(),
                    pixmap.height()
                );

                targetRect.moveCenter(stickerRect.center());
                p.drawPixmap(targetRect, pixmap);
            }
        }

        return QWidget::paintEvent(_e);
    }

    void StickersTable::mousePressEvent(QMouseEvent* _e)
    {
        const auto& stickersList = Stickers::getStickers(stickersSetId_);

        for (uint32_t i = 0; i < stickersList.size(); ++i)
        {
            if (getStickerRect(i).contains(_e->pos()))
            {
                emit stickerSelected(stickersSetId_, stickersList[i]);
            }
        }

        return QWidget::mousePressEvent(_e);
    }


    //////////////////////////////////////////////////////////////////////////
    // RecentsStickersTable
    //////////////////////////////////////////////////////////////////////////
    RecentsStickersTable::RecentsStickersTable(QWidget* _parent)
        :   StickersTable(_parent, -1),
            maxRowCount_(-1)
    {

    }

    RecentsStickersTable::~RecentsStickersTable()
    {

    }

    void RecentsStickersTable::setMaxRowCount(int _val)
    {
        maxRowCount_ = _val;
    }

    bool RecentsStickersTable::resize(const QSize& _size, bool _force)
    {
        const int columnWidth = getStickerItemSize();

        int stickersCount = (int) recentStickersArray_.size();

        bool resized = false;

        if ((prevSize_.width() != _size.width() && _size.width() > columnWidth) || _force)
        {
            columnCount_ = _size.width()/ columnWidth;
            if (columnCount_ > stickersCount)
                columnCount_ = stickersCount;

            rowCount_ = 0;
            if (stickersCount > 0 && columnCount_ > 0)
            {
                rowCount_ = (stickersCount / columnCount_) + (((stickersCount % columnCount_) > 0) ? 1 : 0);
                if (maxRowCount_ > 0 && rowCount_ > maxRowCount_)
                    rowCount_ = maxRowCount_;
            }

            needHeight_ = getStickerItemSize() * rowCount_;

            resized = true;
        }

        prevSize_ = _size;

        return resized;
    }

    void RecentsStickersTable::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);

        for (uint32_t i = 0; i < recentStickersArray_.size(); ++i)
        {
            const auto& stickerRect = getStickerRect(i);
            if (!_e->rect().intersects(stickerRect))
            {
                continue;
            }

            auto image = Stickers::getStickerImage(recentStickersArray_[i].first, recentStickersArray_[i].second, core::sticker_size::small);
            if (image.isNull())
            {
                assert(preloader_);
                preloader_->Draw(p, stickerRect);

                continue;
            }

            if (platform::is_apple() && Utils::is_mac_retina())
            {
                QSize newSize = image.size() / 2;

                if (newSize.width() > stickerRect.size().width() ||
                    newSize.height() > stickerRect.size().height())
                {
                    newSize = image.size().scaled(stickerRect.size(), Qt::KeepAspectRatio);
                }

                QRect targetRect(
                    stickerRect.left(),
                    stickerRect.top(),
                    newSize.width(),
                    newSize.height()
                    );

                targetRect.moveCenter(stickerRect.center());

                p.drawImage(targetRect, image, image.rect());
            }
            else
            {
                auto pixmap = QPixmap::fromImage(image);
                pixmap = pixmap.scaled(stickerRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

                QRect targetRect(
                    stickerRect.left(),
                    stickerRect.top(),
                    pixmap.width(),
                    pixmap.height()
                    );

                targetRect.moveCenter(stickerRect.center());
                p.drawPixmap(targetRect, pixmap);
            }
        }

        return QWidget::paintEvent(_e);
    }

    bool RecentsStickersTable::addSticker(int32_t _setId, int32_t _stickerId)
    {
        for (auto iter = recentStickersArray_.begin(); iter != recentStickersArray_.end(); ++iter)
        {
            if (iter->first == _setId && iter->second == _stickerId)
            {
                if (iter == recentStickersArray_.begin())
                    return false;

                recentStickersArray_.erase(iter);
                break;
            }
        }

        recentStickersArray_.emplace(recentStickersArray_.begin(), _setId, _stickerId);

        if ((int32_t)recentStickersArray_.size() > max_stickers_count)
            recentStickersArray_.pop_back();

        return true;
    }

    void RecentsStickersTable::onStickerUpdated(int32_t _setId, int32_t _stickerId)
    {
        for (uint32_t i = 0; i < recentStickersArray_.size(); ++i)
        {
            if (recentStickersArray_[i].first == _setId && recentStickersArray_[i].second == _stickerId)
            {
                update(getStickerRect(i));
                break;
            }
        }
    }

    void RecentsStickersTable::mousePressEvent(QMouseEvent* _e)
    {
        for (uint32_t i = 0; i < recentStickersArray_.size(); ++i)
        {
            if (getStickerRect(i).contains(_e->pos()))
            {
                emit stickerSelected(recentStickersArray_[i].first, recentStickersArray_[i].second);
                break;
            }
        }

        return QWidget::mousePressEvent(_e);
    }

    const recentStickersArray& RecentsStickersTable::getStickers() const
    {
        return recentStickersArray_;
    }

    //////////////////////////////////////////////////////////////////////////
    // class StickersWidget
    //////////////////////////////////////////////////////////////////////////


    StickersWidget::StickersWidget(QWidget* _parent, Toolbar* _toolbar)
        :	QWidget(_parent), toolbar_(_toolbar), initialized_(false)
    {
    }

    StickersWidget::~StickersWidget()
    {
    }

    void StickersWidget::insertNextSet(int32_t _setId)
    {
        if (setTables_.find(_setId) != setTables_.end())
            return;

        auto stickersView = new StickersTable(this, _setId);
        auto button = toolbar_->addButton(Stickers::getSetIcon(_setId));
        button->AttachView(AttachedView(stickersView, this));

        connect(button, &TabButton::clicked, [this, _setId]()
        {
            emit scrollToSet(setTables_[_setId]->geometry().top());
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::picker_tab_click);
        });

        QLabel* setHeader = new QLabel(this);
        setHeader->setObjectName("set_header");
        setHeader->setText(Stickers::getSetName(_setId));
        vLayout_->addWidget(setHeader);

        vLayout_->addWidget(stickersView);
        setTables_[_setId] = stickersView;
        Utils::grabTouchWidget(stickersView);

        connect(stickersView, &StickersTable::stickerSelected, [this, stickersView](int32_t _setId, int32_t _stickerId)
        {
            emit stickerSelected(_setId, _stickerId);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::sticker_sent_from_picker);
        });
    }

    void StickersWidget::init()
    {
        if (!initialized_)
        {
            vLayout_ = Utils::emptyVLayout();
        }
        
        for (auto stickersSetId : Stickers::getStickersSets())
        {
            insertNextSet(stickersSetId);
        }

        if (!initialized_)
        {
            setLayout(vLayout_);
        }

        initialized_ = true;
    }

    void StickersWidget::onStickerUpdated(int32_t _setId, int32_t _stickerId)
    {
        auto iterSet = setTables_.find(_setId);
        if (iterSet == setTables_.end())
        {
            return;
        }

        iterSet->second->onStickerUpdated(_setId, _stickerId);
    }

    //////////////////////////////////////////////////////////////////////////
    // Recents widget
    //////////////////////////////////////////////////////////////////////////
    RecentsWidget::RecentsWidget(QWidget* _parent)
        :	QWidget(_parent),
        vLayout_(nullptr),
        emojiView_(nullptr),
        stickersView_(nullptr)
    {
        initEmojisFromSettings();

        connect(Ui::GetDispatcher(), SIGNAL(onStickers()), this, SLOT(stickers_event()));
    }

    RecentsWidget::~RecentsWidget()
    {
    }

    void RecentsWidget::stickers_event()
    {
        initStickersFromSettings();
    }

    void RecentsWidget::init()
    {
        if (vLayout_)
            return;

        vLayout_ = Utils::emptyVLayout();

        QLabel* setHeader = new QLabel(this);
        setHeader->setObjectName("set_header");
        setHeader->setText(QT_TRANSLATE_NOOP("input_widget", "RECENTS"));
        vLayout_->addWidget(setHeader);
        
        stickersView_ = new RecentsStickersTable(this);
        stickersView_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        stickersView_->setMaxRowCount(2);
        vLayout_->addWidget(stickersView_);
        Utils::grabTouchWidget(stickersView_);

        emojiView_ = new EmojiTableView(this, new EmojiViewItemModel(this, true));
        emojiView_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        emojiView_->addCategory(emoji_category("recents", emojis_));
        vLayout_->addWidget(emojiView_);
        Utils::grabTouchWidget(emojiView_);

        setLayout(vLayout_);

        connect(emojiView_, &EmojiTableView::clicked, [this](const QModelIndex & _index)
        {
            auto emoji = emojiView_->getEmoji(_index.column(), _index.row());
            if (emoji)
            {
                emit emojiSelected(emoji);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::smile_sent_from_recents);
            }
        });

        connect(stickersView_, &RecentsStickersTable::stickerSelected, [this](qint32 _setId, qint32 _stickerId)
        {
            emit stickerSelected(_setId, _stickerId);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::sticker_sent_from_recents);
        });
    }

    void RecentsWidget::addSticker(int32_t _setId, int32_t _stickerId)
    {
        init();

        if (stickersView_->addSticker(_setId, _stickerId))
        {
            stickersView_->onStickerAdded();

            const auto& recentsStickers = stickersView_->getStickers();

            std::vector<int32_t> vStickers;
            vStickers.reserve(recentsStickers.size());
            for (uint32_t i = 0; i < recentsStickers.size(); ++i)
            {
                vStickers.push_back(recentsStickers[i].first);
                vStickers.push_back(recentsStickers[i].second);
            }

            get_gui_settings()->set_value<std::vector<int32_t>>(settings_recents_stickers, vStickers);
        }
    }

    void RecentsWidget::initStickersFromSettings()
    {
        static int count = 0;

        ++count;

        auto sticks = get_gui_settings()->get_value<std::vector<int32_t>>(settings_recents_stickers, std::vector<int32_t>());
        if (sticks.empty() || (sticks.size() % 2 != 0))
            return;

        init();

        for (uint32_t i = 0; i < sticks.size();)
        {
            int32_t setId = sticks[i];
            int32_t stickerId = sticks[++i];

            ++i;

            stickersView_->addSticker(setId, stickerId);
        }

        stickersView_->onStickerAdded();
    }

    void RecentsWidget::addEmoji(Emoji::EmojiRecordSptr _emoji)
    {
        init();

        for (auto iter = emojis_.begin(); iter != emojis_.end(); iter++)
        {
            if (_emoji->Codepoint_ == (*iter)->Codepoint_ && _emoji->ExtendedCodepoint_ == (*iter)->ExtendedCodepoint_)
            {
                if (iter == emojis_.begin())
                    return;

                emojis_.erase(iter);
                break;
            }
        }

        emojis_.insert(emojis_.begin(), _emoji);
        if (emojis_.size() > 20)
            emojis_.pop_back();

        emojiView_->onEmojiAdded();

        std::vector<int32_t> vEmojis;
        vEmojis.reserve(emojis_.size() * 2);
        for (uint32_t i = 0; i < emojis_.size(); i++)
        {
            vEmojis.push_back(emojis_[i]->Codepoint_);
            vEmojis.push_back(emojis_[i]->ExtendedCodepoint_);
        }

        get_gui_settings()->set_value<std::vector<int32_t>>(settings_recents_emojis, vEmojis);
    }

    void RecentsWidget::initEmojisFromSettings()
    {
        auto emojis = get_gui_settings()->get_value<std::vector<int32_t>>(settings_recents_emojis, std::vector<int32_t>());
        if (emojis.empty() || (emojis.size() % 2 != 0))
            return;

        init();

        emojis_.reserve(20);

        for (uint32_t i = 0; i < emojis.size();)
        {
            int32_t codepoint = emojis[i];
            int32_t extCodepoint = emojis[++i];

            ++i;

            auto emoji = Emoji::GetEmojiInfoByCodepoint(codepoint, extCodepoint);
            if (!emoji)
            {
                assert(false);
                continue;
            }

            emojis_.push_back(emoji);
        }

        emojiView_->onEmojiAdded();
    }

    void RecentsWidget::onStickerUpdated(int32_t _setId, int32_t _stickerId)
    {
        if (stickersView_)
        {
            stickersView_->onStickerUpdated(_setId, _stickerId);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // SmilesMenu class
    //////////////////////////////////////////////////////////////////////////

    SmilesMenu::SmilesMenu(QWidget* _parent)
        : QFrame(_parent)
        , isVisible_(false)
        , currentHeight_(0)
        , topToolbar_(0)
        , bottomToolbar_(0)
        , viewArea_(0)
        , recentsView_(0)
        , emojiView_(0)
        , stickersView_(0)
        , animHeight_(0)
        , animScroll_(0)
        , blockToolbarSwitch_(false)
        , stickerMetaRequested_(false)
    {
        rootVerticalLayout_ = Utils::emptyVLayout(this);
        setLayout(rootVerticalLayout_);

        setStyleSheet(Utils::LoadStyle(":/main_window/smiles_menu/smiles_menu.qss"));

        InitSelector();
        InitStickers();
        InitResents();

        if (Ui::GetDispatcher()->isImCreated())
        {
            im_created();
        }
        
        connect(Ui::GetDispatcher(), SIGNAL(im_created()), this, SLOT(im_created()), Qt::QueuedConnection);
    }

    SmilesMenu::~SmilesMenu()
    {
    }

    void SmilesMenu::im_created()
    {
        int scale = (int) (Utils::getScaleCoefficient() * 100.0);
        scale = Utils::scale_bitmap(scale);
        std::string size = "small";

        switch (scale)
        {
        case 150:
            size = "medium";
            break;
        case 200:
            size = "large";
            break;
        }

        stickerMetaRequested_ = true;

        gui_coll_helper collection(GetDispatcher()->create_collection(), true);
        collection.set_value_as_string("size", size);
        Ui::GetDispatcher()->post_message_to_core("stickers/meta/get", collection.get());
    }

    void SmilesMenu::touchScrollStateChanged(QScroller::State state)
    {
        recentsView_->blockSignals(state != QScroller::Inactive);
        emojiView_->blockSignals(state != QScroller::Inactive);
        stickersView_->blockSignals(state != QScroller::Inactive);
    }

    void SmilesMenu::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive (QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }

    void SmilesMenu::focusOutEvent(QFocusEvent* _event)
    {
        Qt::FocusReason reason = _event->reason();

        if (reason != Qt::FocusReason::ActiveWindowFocusReason)
            Hide();

        QFrame::focusOutEvent(_event);
    }

    void SmilesMenu::focusInEvent(QFocusEvent* _event)
    {
        QFrame::focusInEvent(_event);
    }


    void SmilesMenu::Show()
    {
        if (isVisible_)
            return;

        ShowHide();
    }

    void SmilesMenu::Hide()
    {
        if (!isVisible_)
            return;

        ShowHide();
    }

    void SmilesMenu::ShowHide()
    {
        isVisible_ = !isVisible_;

        auto mainWindow = Utils::InterConnector::instance().getMainWindow();
        int pickerDefaultHeight = Utils::scale_value(320);
        int minTopMargin = Utils::scale_value(160);
        InputWidget* input = Utils::InterConnector::instance().getContactDialog()->getInputWidget();
        int inputWidgetHeight = input->get_current_height();
        auto pickerHeight =
            ((mainWindow->height() - pickerDefaultHeight - inputWidgetHeight) > minTopMargin) ?
            pickerDefaultHeight : (mainWindow->height() - minTopMargin - inputWidgetHeight);
        int start_value = isVisible_ ? 0 : pickerHeight;
        int end_value = isVisible_ ? pickerHeight : 0;

        QEasingCurve easing_curve = QEasingCurve::InQuad;
        int duration = 200;

        if (!animHeight_)
        {
            animHeight_ = new QPropertyAnimation(this, "currentHeight");

            connect(animHeight_, &QPropertyAnimation::finished, [this]()
            {
                if (!isVisible_)
                    Ui::Stickers::clearCache();
            });
        }

        if (isVisible_)
            topToolbar_->updateArrowButtonsVisibility();

        animHeight_->stop();
        animHeight_->setDuration(duration);
        animHeight_->setStartValue(start_value);
        animHeight_->setEndValue(end_value);
        animHeight_->setEasingCurve(easing_curve);
        animHeight_->start();
    }

    bool SmilesMenu::IsHidden() const
    {
        assert(currentHeight_ >= 0);
        return (currentHeight_ == 0);
    }

    void SmilesMenu::ScrollTo(int _pos)
    {
        QEasingCurve easing_curve = QEasingCurve::InQuad;
        int duration = 200;

        if (!animScroll_)
            animScroll_ = new QPropertyAnimation(viewArea_->verticalScrollBar(), "value");

        blockToolbarSwitch_ = true;
        animScroll_->stop();
        animScroll_->setDuration(duration);
        animScroll_->setStartValue(viewArea_->verticalScrollBar()->value());
        animScroll_->setEndValue(_pos);
        animScroll_->setEasingCurve(easing_curve);
        animScroll_->start();

        connect(animScroll_, &QPropertyAnimation::finished, [this]()
        {
            blockToolbarSwitch_ = false;
        });
    }

    void SmilesMenu::InitResents()
    {
        connect(recentsView_, &RecentsWidget::emojiSelected, [this](Emoji::EmojiRecordSptr _emoji)
        {
            emit emojiSelected(_emoji->Codepoint_, _emoji->ExtendedCodepoint_);
        });

        connect(recentsView_, &RecentsWidget::stickerSelected, [this](qint32 _setId, qint32 _stickerId)
        {
            emit stickerSelected(_setId, _stickerId);
        });
    }

    void SmilesMenu::stickersMetaEvent()
    {
        stickerMetaRequested_ = false;
        stickersView_->init();
    }

    void SmilesMenu::stickerEvent(qint32 _setId, qint32 _stickerId)
    {
        stickersView_->onStickerUpdated(_setId, _stickerId);
        recentsView_->onStickerUpdated(_setId, _stickerId);
    }

    void SmilesMenu::InitStickers()
    {
        connect(Ui::GetDispatcher(), SIGNAL(onStickers()), this, SLOT(stickersMetaEvent()));
        connect(Ui::GetDispatcher(), SIGNAL(onSticker(qint32, qint32)), this, SLOT(stickerEvent(qint32, qint32)));

        connect(stickersView_, &StickersWidget::stickerSelected, [this](qint32 _setId, qint32 _stickerId)
        {
            emit stickerSelected(_setId, _stickerId);

            recentsView_->addSticker(_setId, _stickerId);
        });

        connect(stickersView_, &StickersWidget::scrollToSet, [this](int _pos)
        {
            ScrollTo(stickersView_->geometry().top() + _pos);
        });
    }


    void SmilesMenu::InitSelector()
    {
        topToolbar_ = new Toolbar(this, buttons_align::left);
        recentsView_ = new RecentsWidget(this);
        emojiView_ = new EmojisWidget(this);

        topToolbar_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        topToolbar_->setFixedHeight(Utils::scale_value(49));

        auto resents_button = topToolbar_->addButton(":/resources/smiles_menu/picker_tab_recents_100.png");
        resents_button->AttachView(recentsView_);
        connect(resents_button, &TabButton::clicked, [this]()
        {
            ScrollTo(recentsView_->geometry().top());
            emojiView_->selectFirstButton();
        });

        auto emojiButton = topToolbar_->addButton(":/resources/smiles_menu/picker_tab_emoji_100.png");
        emojiButton->AttachView(AttachedView(emojiView_));
        connect(emojiButton, &TabButton::clicked, [this]()
        {
            ScrollTo(emojiView_->geometry().top());
            emojiView_->selectFirstButton();
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::picker_tab_click);
        });

        rootVerticalLayout_->addWidget(topToolbar_);

        viewArea_ = CreateScrollAreaAndSetTrScrollBar(this);
        viewArea_->setFocusPolicy(Qt::NoFocus);
        Utils::grabTouchWidget(viewArea_->viewport(), true);
        connect(QScroller::scroller(viewArea_->viewport()), SIGNAL(stateChanged(QScroller::State)), this, SLOT(touchScrollStateChanged(QScroller::State)), Qt::QueuedConnection);

        QWidget* scroll_area_widget = new QWidget(viewArea_);
        scroll_area_widget->setObjectName("scroll_area_widget");
        viewArea_->setWidget(scroll_area_widget);
        viewArea_->setWidgetResizable(true);
        rootVerticalLayout_->addWidget(viewArea_);


        QVBoxLayout* sa_widgetLayout = new QVBoxLayout();
        sa_widgetLayout->setContentsMargins(Utils::scale_value(10), 0, Utils::scale_value(10), 0);
        scroll_area_widget->setLayout(sa_widgetLayout);

        recentsView_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sa_widgetLayout->addWidget(recentsView_);
        Utils::grabTouchWidget(recentsView_);

        emojiView_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sa_widgetLayout->addWidget(emojiView_);
        Utils::grabTouchWidget(emojiView_);

        stickersView_ = new StickersWidget(this, topToolbar_);
        stickersView_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sa_widgetLayout->addWidget(stickersView_);
        Utils::grabTouchWidget(stickersView_);

        emojiButton->toggle();

        connect(emojiView_, &EmojisWidget::emojiSelected, [this](Emoji::EmojiRecordSptr _emoji)
        {
            emit emojiSelected(_emoji->Codepoint_, _emoji->ExtendedCodepoint_);

            recentsView_->addEmoji(_emoji);
        });

        connect(emojiView_, &EmojisWidget::scrollToGroup, [this](int _pos)
        {
            ScrollTo(emojiView_->geometry().top() + _pos);
        });

        HookScroll();
    }

    void SmilesMenu::HookScroll()
    {
        connect(viewArea_->verticalScrollBar(), &QAbstractSlider::valueChanged, [this](int _value)
        {
            QRect viewPortRect = viewArea_->viewport()->geometry();
            QRect emojiRect = emojiView_->geometry();
            QRect viewPortRectForEmoji(0, _value - emojiRect.top(), viewPortRect.width(), viewPortRect.height() + 1);

            emojiView_->onViewportChanged(viewPortRectForEmoji, blockToolbarSwitch_);

            viewPortRect.adjust(0, _value, 0, _value);

            if (blockToolbarSwitch_)
                return;

            for (TabButton* button : topToolbar_->GetButtons())
            {
                auto view = button->GetAttachedView();
                if (view.get_view())
                {
                    QRect rcView = view.get_view()->geometry();

                    if (view.get_view_parent())
                    {
                        QRect rcViewParent = view.get_view_parent()->geometry();
                        rcView.adjust(0, rcViewParent.top(), 0, rcViewParent.top());
                    }

                    QRect intersectedRect = rcView.intersected(viewPortRect);

                    if (intersectedRect.height() > (viewPortRect.height() / 2))
                    {
                        button->setChecked(true);
                        topToolbar_->scrollToButton(button);
                        break;
                    }
                }
            }
        });

    }

    void SmilesMenu::resizeEvent(QResizeEvent * _e)
    {
        QWidget::resizeEvent(_e);

        if (!viewArea_ || !emojiView_)
            return;

        QRect viewPortRect = viewArea_->viewport()->geometry();
        QRect emojiRect = emojiView_->geometry();

        emojiView_->onViewportChanged(QRect(0, viewArea_->verticalScrollBar()->value() - emojiRect.top(), viewPortRect.width(), viewPortRect.height() + 1),
            blockToolbarSwitch_);
    }

    void SmilesMenu::setCurrentHeight(int _val)
    {
        setMaximumHeight(_val);
        setMinimumHeight(_val);

        currentHeight_ = _val;
    }

    int SmilesMenu::getCurrentHeight() const
    {
        return currentHeight_;
    }

    namespace
    {
        qint32 getEmojiItemSize(bool snaps)
        {
            const auto EMOJI_ITEM_SIZE = 44;
            const auto EMOJI_ITEM_SIZE_SNAPS = 30;
            return Utils::scale_value(snaps ? EMOJI_ITEM_SIZE_SNAPS : EMOJI_ITEM_SIZE);
        }

        qint32 getStickerItemSize()
        {
            const auto STICKER_ITEM_SIZE = 98;
            return Utils::scale_value(STICKER_ITEM_SIZE);
        }

        qint32 getStickerSize()
        {
            const auto STICKER_SIZE = 90;
            return Utils::scale_value(STICKER_SIZE);
        }
    }
}
