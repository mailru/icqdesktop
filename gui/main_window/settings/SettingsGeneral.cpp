#include "stdafx.h"
#include "GeneralSettingsWidget.h"

#include "../../controls/CommonStyle.h"
#include "../../controls/ConnectionSettingsWidget.h"
#include "../../controls/GeneralCreator.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/TransparentScrollBar.h"

#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../main_window/MainWindow.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../../utils/translator.h"

using namespace Ui;

const QList<QString> &getLanguagesStrings()
{
    static QList<QString> slist;
    if (slist.isEmpty())
    {
        slist.push_back(QT_TRANSLATE_NOOP("settings_pages", "ru"));
        slist.push_back(QT_TRANSLATE_NOOP("settings_pages", "en"));
        slist.push_back(QT_TRANSLATE_NOOP("settings_pages", "uk"));
        slist.push_back(QT_TRANSLATE_NOOP("settings_pages", "de"));
        slist.push_back(QT_TRANSLATE_NOOP("settings_pages", "pt"));
        slist.push_back(QT_TRANSLATE_NOOP("settings_pages", "cs"));
        slist.push_back(QT_TRANSLATE_NOOP("settings_pages", "fr"));
        assert(slist.size() == Utils::GetTranslator()->getLanguages().size());
    }
    return slist;
}

QString languageToString(const QString& _code)
{
    auto codes = Utils::GetTranslator()->getLanguages();
    auto strs = getLanguagesStrings();
    assert(codes.size() == strs.size() && "Languages codes != Languages strings (1)");
    auto i = codes.indexOf(_code);
    if (i >= 0 && i < codes.size())
        return strs[i];
    return "";
}

QString stringToLanguage(const QString& _str)
{
    auto codes = Utils::GetTranslator()->getLanguages();
    auto strs = getLanguagesStrings();
    assert(codes.size() == strs.size() && "Languages codes != Languages strings (2)");
    auto i = strs.indexOf(_str);
    if (i >= 0 && i < strs.size())
        return codes[i];
    return "";
}

void GeneralSettingsWidget::Creator::initGeneral(GeneralSettings* _parent, std::map<std::string, Synchronizator>& _collector)
{
    auto scrollArea = CreateScrollAreaAndSetTrScrollBar(_parent);
    scrollArea->setWidgetResizable(true);
    Utils::grabTouchWidget(scrollArea->viewport(), true);

    auto mainWidget = new QWidget(scrollArea);
    Utils::grabTouchWidget(mainWidget);

    auto mainLayout = Utils::emptyVLayout(mainWidget);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(Utils::scale_value(36), 0, Utils::scale_value(36), Utils::scale_value(36));

    scrollArea->setWidget(mainWidget);

    auto layout = Utils::emptyHLayout(_parent);
    layout->addWidget(scrollArea);

    {
        GeneralCreator::addHeader(scrollArea, mainLayout, QT_TRANSLATE_NOOP("settings_pages", "Application"));

        if (platform::is_windows())
        {
            GeneralCreator::addSwitcher(
                0,
                scrollArea,
                mainLayout,
                QT_TRANSLATE_NOOP("settings_pages", "Launch when system starts"),
                Utils::isStartOnStartup(),
                [_parent](bool checked) -> QString
            {
                if (Utils::isStartOnStartup() != checked)
                    Utils::setStartOnStartup(checked);

                if (!checked)
                {
                    if (_parent->launchMinimized_.check_)
                    {
                        _parent->launchMinimized_.check_->blockSignals(true);
                        _parent->launchMinimized_.check_->setChecked(false);
                        _parent->launchMinimized_.check_->blockSignals(false);
                        _parent->launchMinimized_.check_->setEnabled(false);
                        _parent->launchMinimized_.text_->setEnabled(false);
                    }
                }
                else
                {
                    if (_parent->launchMinimized_.check_)
                    {
                        _parent->launchMinimized_.check_->blockSignals(true);
                        _parent->launchMinimized_.check_->setChecked(get_gui_settings()->get_value<bool>(settings_start_minimazed, false));
                        _parent->launchMinimized_.check_->blockSignals(false);
                        _parent->launchMinimized_.check_->setEnabled(true);
                        _parent->launchMinimized_.text_->setEnabled(true);
                    }
                }

                return (checked ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
            });
            _parent->launchMinimized_ = GeneralCreator::addSwitcher(
                0,
                scrollArea,
                mainLayout,
                QT_TRANSLATE_NOOP("settings_pages", "Launch minimized"),
                get_gui_settings()->get_value<bool>(settings_start_minimazed, false),
                [](bool checked) -> QString
            {
                get_gui_settings()->set_value<bool>(settings_start_minimazed, checked);
                return (checked ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
            });
            if (!Utils::isStartOnStartup())
            {
                _parent->launchMinimized_.check_->setChecked(false);
                _parent->launchMinimized_.check_->setEnabled(false);
                _parent->launchMinimized_.text_->setEnabled(false);
            }
            GeneralCreator::addSwitcher(
                0,
                scrollArea,
                mainLayout,
                QT_TRANSLATE_NOOP("settings_pages", "Show taskbar icon"),
                get_gui_settings()->get_value<bool>(settings_show_in_taskbar, true),
                [](bool checked) -> QString
            {
                emit Utils::InterConnector::instance().showIconInTaskbar(checked);
                get_gui_settings()->set_value<bool>(settings_show_in_taskbar, checked);
                return (checked ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
            });
        }

        if (platform::is_apple())
        {
            GeneralCreator::addSwitcher(
                0,
                scrollArea,
                mainLayout,
                QT_TRANSLATE_NOOP("settings_pages", "Show in menu bar"),
                get_gui_settings()->get_value(settings_show_in_menubar, true),
                [](bool checked) -> QString
            {
                if (Utils::InterConnector::instance().getMainWindow())
                    Utils::InterConnector::instance().getMainWindow()->showMenuBarIcon(checked);
                if (get_gui_settings()->get_value(settings_show_in_menubar, true) != checked)
                    get_gui_settings()->set_value(settings_show_in_menubar, checked);
                return (checked ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
            });
        }

        _parent->sounds_ = GeneralCreator::addSwitcher(
            &_collector,
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings_pages", "Sounds"),
            get_gui_settings()->get_value<bool>(settings_sounds_enabled, true),
            [](bool checked) -> QString
        {
            Ui::GetDispatcher()->getVoipController().setMuteSounds(!checked);
            if (get_gui_settings()->get_value<bool>(settings_sounds_enabled, true) != checked)
                get_gui_settings()->set_value<bool>(settings_sounds_enabled, checked);
            return (checked ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
        });
    }
    {
        GeneralCreator::addHeader(scrollArea, mainLayout, QT_TRANSLATE_NOOP("settings_pages", "Chat"));

        GeneralCreator::addSwitcher(
            0,
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings_pages", "Preview images and links"),
            get_gui_settings()->get_value<bool>(settings_show_video_and_images, true),
            [](bool checked)
        {
            if (get_gui_settings()->get_value<bool>(settings_show_video_and_images, true) != checked)
                get_gui_settings()->set_value<bool>(settings_show_video_and_images, checked);
            return (checked ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
        });

        GeneralCreator::addSwitcher(
            0,
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings_pages", "Auto play videos"),
            get_gui_settings()->get_value<bool>(settings_autoplay_video, true),
            [](bool checked)
        {
            if (get_gui_settings()->get_value<bool>(settings_autoplay_video, true) != checked)
                get_gui_settings()->set_value<bool>(settings_autoplay_video, checked);
            return (checked ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
        });

        //		GeneralCreator::addSwitcher(
        //    0,
        //    scrollArea,
        //    mainLayout,
        //    QT_TRANSLATE_NOOP("settings_pages", "Auto-scroll to new messages"),
        //    get_gui_settings()->get_value<bool>(settings_auto_scroll_new_messages, false),
        //    [](bool checked)
        //{
        //    if (get_gui_settings()->get_value<bool>(settings_auto_scroll_new_messages, false) != checked)
        //        get_gui_settings()->set_value<bool>(settings_auto_scroll_new_messages, checked);
        //    return (checked ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
        //});

        GeneralCreator::addChooser(
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings_pages", "Save files to:"),
            QDir::toNativeSeparators(get_gui_settings()->get_value(settings_download_directory, Utils::DefaultDownloadsPath())),
            [_parent](TextEmojiWidget* path)
        {
#ifdef __linux__
            QWidget* parentForDialog = 0;
#else
            QWidget* parentForDialog = _parent;
#endif //__linux__
            auto r = QFileDialog::getExistingDirectory(parentForDialog, QT_TRANSLATE_NOOP("settings_pages", "Choose new path"), QDir::toNativeSeparators(get_gui_settings()->get_value(settings_download_directory, Utils::DefaultDownloadsPath())),
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
            if (r.length())
            {
                path->setText(r);
                get_gui_settings()->set_value(settings_download_directory, QDir::toNativeSeparators(r));
            }
        });

        const auto& keysIndex = Utils::getSendKeysIndex();

        std::vector<QString> values;

        for (const auto& key : keysIndex)
            values.push_back(key.first);

        int currentKey = get_gui_settings()->get_value<int>(settings_key1_to_send_message, KeyToSendMessage::Enter);
        int selectedIndex = 0;

        for (unsigned int i = 0; i < keysIndex.size(); ++i)
        {
            if (currentKey == (int)keysIndex[i].second)
                selectedIndex = i;
        }

        GeneralCreator::addDropper(
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings_pages", "Send message by:"),
            values,
            selectedIndex,
            -1,
            [&keysIndex](QString v, int ix, TextEmojiWidget*)
        {
            get_gui_settings()->set_value<int>(settings_key1_to_send_message, keysIndex[ix].second);

        },
            [](bool) -> QString { return ""; });
    }
    {
        GeneralCreator::addHeader(scrollArea, mainLayout, QT_TRANSLATE_NOOP("settings_pages", "Interface"));

        GeneralCreator::addSwitcher(
            0,
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings_pages", "Chat list compact mode"),
            !get_gui_settings()->get_value<bool>(settings_show_last_message, true),
            [](bool checked) -> QString
        {
            if (get_gui_settings()->get_value<bool>(settings_show_last_message, true) != !checked)
                get_gui_settings()->set_value<bool>(settings_show_last_message, !checked);

            emit Utils::InterConnector::instance().compactModeChanged();
            return (checked ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
        });

        GeneralCreator::addSwitcher(
            0,
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings_pages", "Show snaps"),
            get_gui_settings()->get_value<bool>(settings_show_snaps, false),
            [](bool checked) -> QString
        {
            if (get_gui_settings()->get_value<bool>(settings_show_snaps, false) != checked)
                get_gui_settings()->set_value<bool>(settings_show_snaps, checked);

            emit Utils::InterConnector::instance().showSnapsChanged();
            return (checked ? QT_TRANSLATE_NOOP("settings_pages", "On") : QT_TRANSLATE_NOOP("settings_pages", "Off"));
        });

#ifndef __APPLE__
        auto i = (get_gui_settings()->get_value<double>(settings_scale_coefficient, Utils::getBasicScaleCoefficient()) - 1.f) / .25f; if (i > 3) i = 3;
        std::vector< QString > sc; sc.push_back("100"); sc.push_back("125"); sc.push_back("150"); sc.push_back("200");
        GeneralCreator::addProgresser(
            scrollArea,
            mainLayout,
            sc,
            i,
            [sc](TextEmojiWidget* w, TextEmojiWidget* aw, int i) -> void
        {
            static auto su = get_gui_settings()->get_value(settings_scale_coefficient, Utils::getBasicScaleCoefficient());
            double r = sc[i].toDouble() / 100.f;
            if (fabs(get_gui_settings()->get_value<double>(settings_scale_coefficient, Utils::getBasicScaleCoefficient()) - r) >= 0.25f)
                get_gui_settings()->set_value<double>(settings_scale_coefficient, r);
            w->setText(QString("%1 %2%").arg(QT_TRANSLATE_NOOP("settings_pages", "Interface scale:")).arg(sc[i]), CommonStyle::getTextCommonColor());
            if (fabs(su - r) >= 0.25f)
                aw->setText(QT_TRANSLATE_NOOP("settings_pages", "(Application restart required)"), QColor("#579e1c"));
            else if (fabs(Utils::getBasicScaleCoefficient() - r) < 0.05f)
                aw->setText(QT_TRANSLATE_NOOP("settings_pages", "(Recommended)"), CommonStyle::getTextCommonColor());
            else
                aw->setText(" ", CommonStyle::getTextCommonColor());
        });
#endif

        auto ls = getLanguagesStrings();
        auto lc = languageToString(get_gui_settings()->get_value(settings_language, QString("")));
        auto li = ls.indexOf(lc);
        GeneralCreator::addDropper(
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings_pages", "Language:"),
            ls.toVector().toStdVector(),
            li,
            -1,
            [scrollArea](QString v, int /*ix*/, TextEmojiWidget* ad)
        {
            static auto sl = get_gui_settings()->get_value(settings_language, QString(""));
            {
                auto cl = stringToLanguage(v);
                get_gui_settings()->set_value(settings_language, cl);
                Utils::GetTranslator()->updateLocale();
                if (ad && cl != sl)
                    ad->setText(QT_TRANSLATE_NOOP("settings_pages", "(Application restart required)"), QColor("#579e1c"));
                else if (ad)
                    ad->setText(" ", CommonStyle::getTextCommonColor());
            }
        },
            [](bool) -> QString { return ""; });
    }
     
    {
        GeneralCreator::addHeader(scrollArea, mainLayout, QT_TRANSLATE_NOOP("settings_pages", "Connection settings"));
        _parent->connectionTypeChooser_ = GeneralCreator::addChooser(
            scrollArea,
            mainLayout,
            QT_TRANSLATE_NOOP("settings_pages", "Connection type:"),
            "Auto",
            [_parent](TextEmojiWidget* /*b*/)
        {
            auto connection_settings_widget_ = new ConnectionSettingsWidget(_parent);
            connection_settings_widget_->show();
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::proxy_open);
        });
    }
}
