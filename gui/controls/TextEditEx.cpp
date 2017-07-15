#include "stdafx.h"

#include "../cache/emoji/Emoji.h"
#include "../fonts.h"
#include "../utils/log/log.h"
#include "../utils/Text2DocConverter.h"
#include "../utils/utils.h"
#include "../controls/ContextMenu.h"

#include "TextEditEx.h"

namespace Ui
{
    TextEditEx::TextEditEx(QWidget* _parent, const QFont& _font, const QPalette& _palette, bool _input, bool _isFitToText)
        :	QTextBrowser(_parent),
        index_(0),
        font_(_font),
        prevPos_(-1),
        input_(_input),
        isFitToText_(_isFitToText),
        isCatchEnter_(false),
        add_(0),
        limit_(-1)
    {
        init(_font.pixelSize());
        setPalette(_palette);
        initFlashTimer();
    }

    TextEditEx::TextEditEx(QWidget* _parent, const QFont& _font, const QColor& _color, bool _input, bool _isFitToText)
        : QTextBrowser(_parent)
        , index_(0)
        , font_(_font)
        , color_(_color)
        , prevPos_(-1)
        , input_(_input)
        , isFitToText_(_isFitToText)
        , isCatchEnter_(false)
        , add_(0)
        , limit_(-1)
    {
        init(font_.pixelSize());

        QPalette p = palette();
        p.setColor(QPalette::Text, color_);
        setPalette(p);
        initFlashTimer();
    }
    
    void TextEditEx::limitCharacters(int count)
    {
        limit_ = count;
    }

    void TextEditEx::init(int /*_fontSize*/)
    {
        setAcceptRichText(false);
        setFont(font_);
        document()->setDefaultFont(font_);

        QPalette p = palette();
        p.setColor(QPalette::Highlight, Utils::getSelectionColor());
        setPalette(p);

        if (isFitToText_)
        {
            connect(this->document(), SIGNAL(contentsChanged()), this, SLOT(editContentChanged()), Qt::QueuedConnection);
        }
    }

    void TextEditEx::initFlashTimer()
    {
        flashInterval_ = 0;
        flashChangeTimer_ = new QTimer(this);
        flashChangeTimer_->setInterval(500);
        flashChangeTimer_->setSingleShot(true);
        connect(flashChangeTimer_, SIGNAL(timeout()), this, SLOT(enableFlash()), Qt::QueuedConnection);
    }

    void TextEditEx::setFlashInterval(int _interval)
    {
        QApplication::setCursorFlashTime(_interval);
        Qt::TextInteractionFlags f = textInteractionFlags();
        setTextInteractionFlags(0);
        setTextInteractionFlags(f);
    }

    void TextEditEx::editContentChanged()
    {
        int docHeight = this->document()->size().height();

        if (docHeight < Utils::scale_value(20) || docHeight > Utils::scale_value(250))
        {
            return;
        }

        auto oldHeight = height();

        setFixedHeight(docHeight + add_);

        if (height() != oldHeight)
        {
            emit setSize(0, height() - oldHeight);
        }
    }

    void TextEditEx::enableFlash()
    {
        setFlashInterval(flashInterval_);
    }

    QString TextEditEx::getPlainText(int _from, int _to) const
    {
        if (_from == _to)
            return "";

        if (_to != -1 && _to < _from)
        {
            assert(!"invalid data");
            return "";
        }

        QString outString;
        QTextStream result(&outString);

        int posStart = 0;
        int length = 0;

        bool first = true;

        for (QTextBlock it_block = document()->begin(); it_block != document()->end(); it_block = it_block.next())
        {
            if (!first)
                result << '\n';

            posStart = it_block.position();
            if (_to != -1 && posStart >= _to)
                break;

            for (QTextBlock::iterator itFragment = it_block.begin(); itFragment != it_block.end(); ++itFragment)
            {
                QTextFragment currentFragment = itFragment.fragment();

                if (currentFragment.isValid())
                {
                    posStart = currentFragment.position();
                    length = currentFragment.length();

                    if (posStart + length <= _from)
                        continue;

                    if (_to != -1 && posStart >= _to)
                        break;

                    first = false;

                    if (currentFragment.charFormat().isImageFormat())
                    {
                        if (posStart < _from)
                            continue;

                        QTextImageFormat imgFmt = currentFragment.charFormat().toImageFormat();

                        auto iter = resourceIndex_.find(imgFmt.name());
                        if (iter != resourceIndex_.end())
                            result << iter->second;
                    }
                    else
                    {
                        QString fragmentText = currentFragment.text();

                        int cStart = std::max((_from - posStart), 0);
                        int count = -1;
                        if (_to != -1 && _to <= posStart + length)
                            count = _to - posStart - cStart;

                        QString txt = fragmentText.mid(cStart, count);
                        txt.remove(QChar::SoftHyphen);

                        QChar *uc = txt.data();
                        QChar *e = uc + txt.size();

                        for (; uc != e; ++uc)
                        {
                            switch (uc->unicode())
                            {
                            case 0xfdd0: // QTextBeginningOfFrame
                            case 0xfdd1: // QTextEndOfFrame
                            case QChar::ParagraphSeparator:
                            case QChar::LineSeparator:
                                *uc = QLatin1Char('\n');
                                break;
                            case QChar::Nbsp:
                                *uc = QLatin1Char(' ');
                                break;
                            default:
                                ;
                            }
                        }

                        result << txt;
                    }
                }
            }
        }

        return outString;
    }

    QMimeData* TextEditEx::createMimeDataFromSelection() const
    {
        QMimeData* dt =  new QMimeData();

        dt->setText(getPlainText(textCursor().selectionStart(), textCursor().selectionEnd()));

        return dt;
    }

    void TextEditEx::insertFromMimeData(const QMimeData* _source)
    {
        if (_source->hasImage() && !Utils::haveText(_source))
            return;

        if (_source->hasUrls())
        {
            for (auto url : _source->urls())
            {
                if (url.isLocalFile())
                    return;
            }
        }
        if (_source->hasText())
        {
            if (limit_ != -1 && document()->characterCount() >= limit_)
                return;
            
            QString text = _source->text();
            if (limit_ != -1 && text.length() + document()->characterCount() > limit_)
                text.resize(limit_ - document()->characterCount());
            
            if (input_)
            {
                if (platform::is_apple())
                {
                    Logic::Text4Edit(text, *this, Logic::Text2DocHtmlMode::Pass, false);
                    /*
                    auto cursor = textCursor();
                    cursor.beginEditBlock();
                    cursor.insertText(text);
                    cursor.endEditBlock();
                    */
                }
                else
                {
                    Logic::Text4EditEmoji(text, *this);
                }
            }
            else
            {
                Logic::Text4Edit(text, *this, Logic::Text2DocHtmlMode::Pass, false);
            }
        }
    }

    void TextEditEx::insertPlainText_(const QString& _text)
    {
        if (!_text.isEmpty())
        {
            Logic::Text4Edit(_text, *this, Logic::Text2DocHtmlMode::Pass, false);
        }
    }

    bool TextEditEx::canInsertFromMimeData(const QMimeData* _source) const
    {
        return QTextBrowser::canInsertFromMimeData(_source);
    }

    void TextEditEx::focusInEvent(QFocusEvent* _event)
    {
        emit focusIn();
        QTextBrowser::focusInEvent(_event);
    }

    void TextEditEx::focusOutEvent(QFocusEvent* _event)
    {
        if (_event->reason() != Qt::ActiveWindowFocusReason)
            emit focusOut();

        QTextBrowser::focusOutEvent(_event);
    }

    void TextEditEx::mousePressEvent(QMouseEvent* _event)
    {
        if (_event->source() == Qt::MouseEventNotSynthesized)
        {
            emit clicked();
            QTextBrowser::mousePressEvent(_event);
            if (!input_)
                _event->ignore();
        }
    }

    void TextEditEx::mouseReleaseEvent(QMouseEvent* _event)
    {
        QTextBrowser::mouseReleaseEvent(_event);
        if (!input_)
            _event->ignore();
    }

    void TextEditEx::mouseMoveEvent(QMouseEvent * _event)
    {
        if (_event->buttons() & Qt::LeftButton && !input_)
        {
            _event->ignore();
        }
        else
        {
            QTextBrowser::mouseMoveEvent(_event);
        }
    }

    bool TextEditEx::catchEnter(int /*_modifiers*/)
    {
        return isCatchEnter_;
    }

    void TextEditEx::keyPressEvent(QKeyEvent* _event)
    {
        emit keyPressed(_event->key());

        if (_event->key() == Qt::Key_Backspace && toPlainText().isEmpty())
            emit emptyTextBackspace();

        if (_event->key() == Qt::Key_Escape)
            emit escapePressed();

        if (_event->key() == Qt::Key_Up)
            emit upArrow();

        if (_event->key() == Qt::Key_Down)
            emit downArrow();

        if (_event->key() == Qt::Key_Return || _event->key() == Qt::Key_Enter)
        {
            Qt::KeyboardModifiers modifiers = _event->modifiers();

            if (catchEnter(modifiers))
            {
                emit enter();
                return;
            }
        }

        if (platform::is_apple() && input_ && _event->key() == Qt::Key_Backspace && _event->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier))
        {
            auto cursor = textCursor();
            cursor.select(QTextCursor::LineUnderCursor);
            cursor.removeSelectedText();
            //setTextCursor(cursor);

            QTextBrowser::keyPressEvent(_event);
            _event->ignore();

            return;
        }
        
        const auto lenght = document()->characterCount();
        bool deny = limit_ != -1 && lenght >= limit_ && !_event->text().isEmpty();

        auto oldPos = textCursor().position();
        QTextBrowser::keyPressEvent(_event);
        auto newPos = textCursor().position();
        
        if (deny && document()->characterCount() > lenght)
            textCursor().deletePreviousChar();
        
        if (_event->modifiers() & Qt::ShiftModifier && _event->key() == Qt::Key_Up && oldPos == newPos)
        {
            auto cur = textCursor();
            cur.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
            setTextCursor(cur);
        }

        if (oldPos != newPos && _event->modifiers() == Qt::NoModifier)
        {
            if (flashInterval_ == 0)
                flashInterval_ = QApplication::cursorFlashTime();

            setFlashInterval(0);
            flashChangeTimer_->start();
        }
        _event->ignore();
    }

    QString TextEditEx::getPlainText() const
    {
        return getPlainText(0, -1);
    }

    void TextEditEx::setPlainText(const QString& _text, bool _convertLinks, const QTextCharFormat::VerticalAlignment _aligment)
    {
        clear();
        resourceIndex_.clear();

        Logic::Text4Edit(_text, *this, Logic::Text2DocHtmlMode::Escape, _convertLinks, false, nullptr, Emoji::EmojiSizePx::Auto, _aligment);
    }

    void TextEditEx::insertEmoji(int _main, int _ext)
    {
        mergeResources(Logic::InsertEmoji(_main, _ext, *this));
    }

    void TextEditEx::selectWholeText()
    {
        auto cursor = textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    }

    void TextEditEx::selectFromBeginning(const QPoint& _p)
    {
        const auto cursorTo = cursorForPosition(mapFromGlobal(_p));
        const auto posTo = cursorTo.position();

        prevPos_ = posTo;

        auto cursor = textCursor();

        auto isCursorChanged = false;

        const auto isCursorAtTheBeginning = (cursor.position() == 0);
        if (!isCursorAtTheBeginning)
        {
            isCursorChanged = cursor.movePosition(QTextCursor::Start);
            assert(isCursorChanged);
        }

        const auto isCursorAtThePos = (cursor.position() == posTo);
        if (!isCursorAtThePos)
        {
            cursor.setPosition(posTo, QTextCursor::KeepAnchor);
            isCursorChanged = true;
        }

        if (isCursorChanged)
        {
            setTextCursor(cursor);
        }
    }

    void TextEditEx::selectTillEnd(const QPoint& _p)
    {
        const auto cursorTo = cursorForPosition(mapFromGlobal(_p));
        const auto posTo = cursorTo.position();

        prevPos_ = posTo;

        auto cursor = textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.setPosition(posTo, QTextCursor::KeepAnchor);
        setTextCursor(cursor);
    }

    void TextEditEx::selectByPos(const QPoint& _p)
    {
        const auto cursorP = cursorForPosition(mapFromGlobal(_p));
        const auto posP = cursorP.position();

        auto cursor = textCursor();

        if (!cursor.hasSelection() && prevPos_ == -1)
        {
            cursor.setPosition(posP);
            cursor.setPosition(posP > prevPos_ ? posP : posP - 1, QTextCursor::KeepAnchor);
        }
        else
        {
            cursor.setPosition(posP, QTextCursor::KeepAnchor);
        }

        prevPos_ = posP;

        setTextCursor(cursor);
    }

    void TextEditEx::selectByPos(const QPoint& _from, const QPoint& _to)
    {
        auto cursorFrom = cursorForPosition(mapFromGlobal(_from));

        const auto cursorTo = cursorForPosition(mapFromGlobal(_to));
        const auto posTo = cursorTo.position();

        cursorFrom.setPosition(posTo, QTextCursor::KeepAnchor);

        prevPos_ = posTo;

        setTextCursor(cursorFrom);
    }

    void TextEditEx::clearSelection()
    {
        QTextCursor cur = textCursor();
        cur.clearSelection();
        setTextCursor(cur);
        prevPos_ = -1;
    }

    bool TextEditEx::isAllSelected()
    {
        QTextCursor cur = textCursor();
        if (!cur.hasSelection())
            return false;

        const int start = cur.selectionStart();
        const int end = cur.selectionEnd();
        cur.setPosition(start);
        if (cur.atStart())
        {
            cur.setPosition(end);
            return cur.atEnd();
        }
        else if (cur.atEnd())
        {
            cur.setPosition(start);
            return cur.atStart();
        }

        return false;
    }

    QString TextEditEx::selection()
    {
        return getPlainText(textCursor().selectionStart(), textCursor().selectionEnd());
    }

    QSize TextEditEx::getTextSize() const
    {
        return document()->documentLayout()->documentSize().toSize();
    }

    int32_t TextEditEx::getTextHeight() const
    {
        return getTextSize().height();
    }

    int32_t TextEditEx::getTextWidth() const
    {
        return getTextSize().width();
    }

    void TextEditEx::mergeResources(const ResourceMap& _resources)
    {
        for (auto iter = _resources.cbegin(); iter != _resources.cend(); iter++)
            resourceIndex_[iter->first] = iter->second;
    }

    QSize TextEditEx::sizeHint() const
    {
        QSize sizeRect(document()->idealWidth(), document()->size().height());

        return sizeRect;
    }

    void TextEditEx::setCatchEnter(bool _isCatchEnter)
    {
        isCatchEnter_ = _isCatchEnter;
    }

    int TextEditEx::adjustHeight(int _width)
    {
        setFixedWidth(_width);
        document()->setTextWidth(_width);
        int height = getTextSize().height();
        setFixedHeight(height + add_);

        return height;
    }

    void TextEditEx::mousePress(QMouseEvent* ev)
    {
        mousePressEvent(ev);
    }

    void TextEditEx::mouseRelease(QMouseEvent* ev)
    {
        mouseReleaseEvent(ev);
    }

    void TextEditEx::contextMenuEvent(QContextMenuEvent *e)
    {
        auto menu = createStandardContextMenu();
        if (!menu)
            return;
        
        ContextMenu::applyStyle(menu, false, Utils::scale_value(14), Utils::scale_value(24));
        menu->exec(e->globalPos());
        menu->deleteLater();
    }
}

