#pragma once

#include "../namespaces.h"

#include "../utils/utils.h"

FONTS_NS_BEGIN

enum class FontFamily;
enum class FontWeight;

FONTS_NS_END

namespace Ui
{
    class TextEditEx : public QTextBrowser
    {
        Q_OBJECT

    Q_SIGNALS:

        void focusIn();
        void focusOut();
        void clicked();
        void emptyTextBackspace();
        void escapePressed();
        void upArrow();
        void downArrow();
        void enter();
        void setSize(int, int);
        void keyPressed(int);

    private Q_SLOTS:
        void editContentChanged();
        void enableFlash();

    private:

        int index_;

        typedef std::map<QString, QString>	ResourceMap;

        ResourceMap resourceIndex_;
        QFont font_;
        QColor color_;
        int prevPos_;
        bool input_;
        bool isFitToText_;
        int flashInterval_;
        int add_;
        int limit_;

        QTimer* flashChangeTimer_;

        bool isCatchEnter_;

        QString getPlainText(int _from, int _to = -1) const;
        void init(int _fontSize);
        void initFlashTimer();
        void setFlashInterval(int _interval);

    public:

        TextEditEx(QWidget* _parent, const QFont& _font, const QPalette& _palette, bool _input, bool _isFitToText);
        TextEditEx(QWidget* _parent, const QFont& _font, const QColor& _color, bool _input, bool _isFitToText);
        
        void limitCharacters(int count);

        virtual QSize sizeHint() const override;

        QString getPlainText() const;
        void setPlainText(const QString& _text, bool _convertLinks = true, const QTextCharFormat::VerticalAlignment _aligment = QTextCharFormat::AlignBaseline);

        void mergeResources(const ResourceMap& _resources);
        void insertEmoji(int _main, int _ext);
        void insertPlainText_(const QString& _text);

        void selectWholeText();
        void selectFromBeginning(const QPoint& _p);
        void selectTillEnd(const QPoint& _p);
        void selectByPos(const QPoint& _p);
        void selectByPos(const QPoint& _from, const QPoint& _to);
        void clearSelection();
        bool isAllSelected();
        QString selection();

        QSize getTextSize() const;
        int32_t getTextHeight() const;
        int32_t getTextWidth() const;

        void setCatchEnter(bool _isCatchEnter);
        virtual bool catchEnter(int _modifiers);

        int adjustHeight(int _width);

        void addSpace(int _space) { add_ = _space; }

        void mousePress(QMouseEvent* ev);
        void mouseRelease(QMouseEvent* ev);

    protected:
        virtual void focusInEvent(QFocusEvent*) override;
        virtual void focusOutEvent(QFocusEvent*) override;
        virtual void mousePressEvent(QMouseEvent*) override;
        virtual void mouseReleaseEvent(QMouseEvent*) override;
        virtual void mouseMoveEvent(QMouseEvent *) override;
        virtual void keyPressEvent(QKeyEvent*) override;

        virtual QMimeData* createMimeDataFromSelection() const override;
        virtual bool canInsertFromMimeData(const QMimeData* _source) const override;
        virtual void insertFromMimeData(const QMimeData* _source) override;
        virtual void contextMenuEvent(QContextMenuEvent *e) override;
    };
}
