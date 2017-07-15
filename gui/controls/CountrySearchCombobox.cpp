#include "stdafx.h"
#include "CountrySearchCombobox.h"

#include "CustomButton.h"
#include "LineEditEx.h"
#include "PictureWidget.h"
#include "../utils/utils.h"
#ifdef __APPLE__
#   include "../utils/macos/mac_support.h"
#endif

namespace Ui
{
	SearchComboboxView::SearchComboboxView(QWidget* _parent)
		: QTreeView(_parent)
	{
		setStyleSheet(Utils::LoadStyle(":/main_window/login_page.qss"));
	}

	void SearchComboboxView::paintEvent(QPaintEvent* _event)
	{
		int expectedWidth = width() * 0.65;
		if (header()->sectionSize(0) != expectedWidth)
			header()->resizeSection(0, width() * 0.65);
		QTreeView::paintEvent(_event);
	}

	CountrySearchCombobox::CountrySearchCombobox(QWidget* _parent)
		: Edit_(new LineEditEx(_parent))
		, Completer_(new QCompleter(_parent))
		, ComboboxView_(new SearchComboboxView(_parent))
	{
		initLayout();

		ComboboxView_->setSelectionBehavior(QAbstractItemView::SelectRows);
		ComboboxView_->setAllColumnsShowFocus(true);
		ComboboxView_->setRootIsDecorated(false);
		ComboboxView_->header()->hide();
        
        searchGlass_ = new PictureWidget(Edit_, ":/resources/i_search_100.png");
        searchGlass_->setFixedWidth(Utils::scale_value(20));
        searchGlass_->setFixedHeight(Utils::scale_value(20));
        searchGlass_->hide();
        searchGlass_->setAttribute(Qt::WA_TransparentForMouseEvents);
        
        dropDown_ = new CustomButton(Edit_, ":/resources/basic_elements/arrow_small_a_100.png");
        dropDown_->setFixedWidth(Utils::scale_value(20));
        dropDown_->setFixedHeight(Utils::scale_value(20));
        dropDown_->setAttribute(Qt::WA_TransparentForMouseEvents);

		connect(Edit_, SIGNAL(focusIn()), this, SLOT(setFocusIn()), Qt::QueuedConnection);
		connect(Edit_, SIGNAL(focusOut()), this, SLOT(setFocusOut()), Qt::QueuedConnection);
		connect(Edit_, SIGNAL(clicked()), this, SLOT(editClicked()), Qt::QueuedConnection);
		connect(Edit_, SIGNAL(textEdited(QString)), this, SLOT(editTextChanged(QString)), Qt::QueuedConnection);
		connect(Edit_, SIGNAL(editingFinished()), this, SLOT(editCompleted()), Qt::QueuedConnection);
		connect(Completer_, SIGNAL(activated(QString)), this, SLOT(completerActivated(QString)), Qt::QueuedConnection);
        
        Edit_->setAttribute(Qt::WA_MacShowFocusRect, false);
        this->setAttribute(Qt::WA_MacShowFocusRect, false);
	}
    
    void CountrySearchCombobox::setFocusIn()
    {
        searchGlass_->show();
    }
    
    void CountrySearchCombobox::setFocusOut()
    {
        searchGlass_->hide();
    }
    
    void CountrySearchCombobox::resizeEvent(QResizeEvent *_e)
    {
        QWidget::resizeEvent(_e);
		QRect r = rect();
        searchGlass_->move(r.x() + Utils::scale_value(4), r.y() + Utils::scale_value(14));
        dropDown_->move(r.width() - dropDown_->width(), r.y() + Utils::scale_value(16));
    }

	void CountrySearchCombobox::initLayout()
	{
		QHBoxLayout* mainLayout = Utils::emptyHLayout(this);
		mainLayout->addWidget(Edit_);
		QSpacerItem* editLayoutSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum);
		mainLayout->addSpacerItem(editLayoutSpacer);
		setLayout(mainLayout);
	}

	void CountrySearchCombobox::setComboboxViewClass(const char* _className)
	{
		ComboboxView_->setProperty(_className, true);
	}

	void CountrySearchCombobox::setClass(const char* _className)
	{
		setProperty(_className, true);
	}

	void CountrySearchCombobox::setSources(QMap<QString, QString> _sources)
	{
		Sources_ = _sources;
		QStandardItemModel* model = new QStandardItemModel(this);
		model->setColumnCount(2);
		int i = 0;
		for (auto iter : _sources.toStdMap())
		{
			QStandardItem* firstCol = new QStandardItem(iter.first);
			QStandardItem* secondCol = new QStandardItem(iter.second);
			secondCol->setTextAlignment(Qt::AlignRight);
			model->setItem(i, 0, firstCol);
			model->setItem(i, 1, secondCol);
			++i;
		}

		Completer_->setCaseSensitivity(Qt::CaseInsensitive);
		Completer_->setModel(model);
		Completer_->setPopup(ComboboxView_);
		Completer_->setCompletionColumn(0);
		Completer_->setCompletionMode(QCompleter::PopupCompletion);
		Edit_->setCompleter(Completer_);
	}

	void CountrySearchCombobox::setPlaceholder(QString _placeholder)
	{
		Edit_->setPlaceholderText(_placeholder);
	}

	void CountrySearchCombobox::editClicked()
	{
		if (Edit_->completer())
		{
			OldEditValue_ = Edit_->text();
			Edit_->clear();
			Completer_->setCompletionPrefix(QString());
			Edit_->completer()->complete();
		}
	}

	void CountrySearchCombobox::completerActivated(QString _text)
	{
		Edit_->clearFocus();
	}

	void CountrySearchCombobox::editCompleted()
	{
		QString value;
		if (Completer_->completionColumn() == 0)
		{
			value = getValue(Edit_->text());
		}
		else
		{
			value = Edit_->text();
			QRegExp re("\\d*");
			if (re.exactMatch(value))
				value = "+" + value;

            bool doClassicRoutine = true;
            if (auto selectionModel = ComboboxView_->selectionModel())
            {
                auto selectedRows = selectionModel->selectedRows();
                if (!selectedRows.isEmpty())
                {
                    doClassicRoutine = false;
                    auto rowModel = selectedRows.first();
                    auto country = rowModel.data().toString();
                    Edit_->setText(country);
                }
            }
            if (doClassicRoutine)
            {
                if (value == Utils::GetTranslator()->getCurrentPhoneCode())
                {
#ifdef __APPLE__
                    QString counryCode = MacSupport::currentRegion().right(2).toLower();;
#else
                    QString counryCode = QLocale::system().name().right(2).toLower();
#endif
                    const auto mustBe = Utils::getCountryNameByCode(counryCode).toLower();
                    const auto stdMap = Sources_.toStdMap();
                    const auto it = std::find_if(
                        stdMap.begin(),
                        stdMap.end(),
                        [mustBe](const std::pair<QString, QString> &p)
                    {
                        return p.first.toLower() == mustBe;
                    });
                    if (it != stdMap.end())
                        Edit_->setText(it->first);
                    else
                        Edit_->setText(Sources_.key(value));
                }
                else
                {
                    Edit_->setText(Sources_.key(value));
                }
            }
			Completer_->setCompletionColumn(0);
		}
		
		if (value.isEmpty())
		{
			Completer_->setCompletionPrefix(QString());
			Edit_->setText(OldEditValue_);
		}
		else
		{
			emit selected(value);
		}
	}

	void CountrySearchCombobox::editTextChanged(QString _text)
	{
		int completionColumn = Completer_->completionColumn();
		int newCompletionColumn;
		QRegExp re("[\\+\\d]\\d*");
		if (re.exactMatch(_text))
		{
			QString completion = Completer_->completionPrefix();
			if (!completion.startsWith("+"))
				Completer_->setCompletionPrefix("+" + completion);
			newCompletionColumn = 1;
		}
		else
		{
			newCompletionColumn = 0;
		}

		if (completionColumn != newCompletionColumn)
		{
            Completer_->setCompletionColumn(newCompletionColumn);
		}

		if (_text.isEmpty())
		{
			Completer_->setCompletionPrefix(QString());
		}
		else
		{
			Completer_->complete();
		}
	}

	QString CountrySearchCombobox::getValue(const QString& _key)
	{
		for (auto iter : Sources_.uniqueKeys())
		{
			if (iter.compare(_key, Qt::CaseInsensitive) == 0)
			{
				return Sources_[iter];
			}
		}

		return QString();
	}

	bool CountrySearchCombobox::selectItem(QString _item)
	{
		QString value;
		QRegExp re("[\\+\\d]\\d*");
		if (re.exactMatch(_item))
        {
            _item = _item.startsWith("+") ? _item : ("+" + _item);
            if (_item == Utils::GetTranslator()->getCurrentPhoneCode())
            {
#ifdef __APPLE__
                QString counryCode = MacSupport::currentRegion().right(2).toLower();;
#else
                QString counryCode = QLocale::system().name().right(2).toLower();
#endif
                const auto mustBe = Utils::getCountryNameByCode(counryCode).toLower();
                const auto stdMap = Sources_.toStdMap();
                const auto it = std::find_if(stdMap.begin(), stdMap.end(), [mustBe](const std::pair<QString, QString> &p){ return p.first.toLower() == mustBe; });
                if (it != stdMap.end())
                    _item = it->first;
                else
                    _item = Sources_.key(_item);
            }
            else
            {
                _item = Sources_.key(_item);
            }
        }
		
		value = getValue(_item);
		if (!value.isEmpty())
		{
			Edit_->setText(_item);
			OldEditValue_ = _item;
			emit selected(value);
			return true;
		}
		return false;
	}

	bool CountrySearchCombobox::containsCode(QString _code)
	{
		for (auto iter : Sources_)
		{
			if (iter.indexOf(_code) != -1)
			{
				return true;
			}
		}

		return false;
	}
}
