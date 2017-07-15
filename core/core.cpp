#include "stdafx.h"
#include "core.h"
#include "async_task.h"
#include "main_thread.h"
#include "network_log.h"
#include "configuration/app_config.h"
#include "configuration/hosts_config.h"

#include "connections/im_container.h"
#include "connections/base_im.h"
#include "connections/login_info.h"
#include "connections/im_login.h"

#include "Voip/VoipManager.h"
#include "Voip/libvoip/include/voip/voip2.h"

#include "../corelib/collection_helper.h"
#include "core_settings.h"
#include "curl_handler.h"
#include "gui_settings.h"
#include "themes/theme_settings.h"
#include "utils.h"
#include "http_request.h"
#include "scheduler.h"
#include "archive/local_history.h"
#include "log/log.h"
#include "profiling/profiler.h"
#include "updater/updater.h"
#include "crash_sender.h"
#include "statistics.h"
#include "tools/md5.h"
#include "../corelib/enumerations.h"
#include "../common.shared/common.h"
#include "tools/system.h"
#include "proxy_settings.h"
#include "tools/strings.h"

#ifdef _WIN32
    #include "../common.shared/win32/crash_handler.h"
#endif

using namespace core;

std::unique_ptr<core::core_dispatcher>	core::g_core;

int32_t build::is_core_icq = 0;

core_dispatcher::core_dispatcher()
    : gui_connector_(nullptr)
    , core_factory_(nullptr)
    , core_thread_(nullptr)
    , delayed_stat_timer_id_(0)
{
    http_request_simple::init_global();

    curl_handler::instance().init();

#ifdef _WIN32
    std::locale loc = boost::locale::generator().generate("");
    std::locale::global(loc);
#endif

    search_count_.store(0);

    profiler::enable(::build::is_debug());
}


core_dispatcher::~core_dispatcher()
{
    profiler::flush_logs();

    __LOG(log::shutdown();)

    curl_handler::instance().cleanup();

    http_request_simple::shutdown_global();
}

std::string core::core_dispatcher::get_uniq_device_id()
{
    return settings_->get_value<std::string>(core_settings_values::csv_device_id, std::string());
}

void core::core_dispatcher::execute_core_context(std::function<void()> _func)
{
    if (!core_thread_)
    {
        assert(!"core thread empty");
        return;
    }

    core_thread_->execute_core_context(_func);
}

std::shared_ptr<base_im> core::core_dispatcher::find_im_by_id(unsigned id) {
    assert(!!im_container_);
    if (!!im_container_) {
        return im_container_->get_im_by_id(id);
    }
    return NULL;
}

uint32_t core::core_dispatcher::add_timer(std::function<void()> _func, uint32_t _timeout)
{
    if (!scheduler_)
    {
        assert(false);
        return 0;
    }

    return scheduler_->push_timer(_func, _timeout);
}

void core::core_dispatcher::stop_timer(uint32_t _id)
{
    if (scheduler_)
        scheduler_->stop_timer(_id);
}

std::shared_ptr<async_task_handlers> core::core_dispatcher::save_async(std::function<int32_t()> task)
{
    assert(!!save_thread_);
    if (!save_thread_)
        return std::make_shared<async_task_handlers>();
    return save_thread_->run_async_function(task);
}


void core::core_dispatcher::link_gui(icore_interface* _core_face, const common::core_gui_settings& _settings)
{
    // called from main thread

    build::set_core(_settings.is_build_icq_);

    gui_connector_ = _core_face->get_gui_connector();
    core_factory_ = _core_face->get_factory();
    core_thread_ = new main_thread();

    execute_core_context([this, _settings]
    {
        start(_settings);
    });
}

void core::core_dispatcher::start(const common::core_gui_settings& _settings)
{
    __LOG(log::init(utils::get_logs_path(), false);)

    core_gui_settings_ = _settings;

    boost::system::error_code error_code;
    const auto product_data_root = boost::filesystem::canonical(utils::get_product_data_path(), Out error_code);
    const auto app_ini_path = boost::filesystem::canonical(product_data_root / L"app.ini", Out error_code);
    configuration::load_app_config(app_ini_path);

    // called from core thread
    network_log_.reset(new network_log(utils::get_logs_path()));

    settings_ = std::make_shared<core::core_settings>(product_data_root / L"settings/core.stg"
        , utils::get_product_data_path() + L"/settings/" + tools::from_utf8(core_settings_export_file_name));
    settings_->load();

    if (settings_->end_init_default())
    {
        settings_->save();
    }

    proxy_settings_manager_.reset(new proxy_settings_manager(*settings_));

    save_thread_.reset(new async_executer());
    scheduler_.reset(new scheduler());

    load_gui_settings();
    load_theme_settings();
    load_hosts_config();

    post_need_promo();
    post_theme_settings();
    post_gui_settings();
    post_app_config();
    post_user_proxy_to_gui();

    curl_handler::instance().start();

#ifndef STRIP_VOIP
    voip_manager_.reset(new(std::nothrow) voip_manager::VoipManager(*this));
    assert(!!voip_manager_);
#endif //__STRIP_VOIP
    im_container_.reset(new im_container(voip_manager_));

    if (!settings_->get_need_show_promo())
        im_container_->create();

    post_logins();

    updater_.reset(new update::updater());

#ifdef _WIN32
    core::dump::set_os_version(core::tools::system::get_os_version_string());
    report_sender_.reset(new dump::report_sender(g_core->get_login_after_start()));
    report_sender_->send_report();
#endif

    load_statistics();
}


void core::core_dispatcher::unlink_gui()
{
    curl_handler::instance().stop();

    execute_core_context([this]
    {
        // NOTE : this order is important!
        voip_manager_.reset();
        im_container_.reset();
        gui_settings_.reset();
        scheduler_.reset();
        hosts_config_.reset();
        if (is_stats_enabled())
            statistics_.reset();
        save_thread_.reset();
        updater_.reset();
        report_sender_.reset();
        network_log_.reset();
        proxy_settings_manager_.reset();
        theme_settings_.reset();
    });

    delete core_thread_;
    core_thread_ = nullptr;

    gui_connector_->release();
    gui_connector_ = nullptr;

    core_factory_->release();
    core_factory_ = nullptr;
}


void core::core_dispatcher::post_message_to_gui(const char * _message, int64_t _seq, icollection* _message_data)
{
    tools::binary_stream bs;
    bs.write<std::string>("CORE->GUI: message=");
    bs.write<std::string>(_message);
    bs.write<std::string>("\r\n");

	// Added type of voip call to log.
	if (strcmp(_message, "voip_signal") == 0 && _message_data)
	{
		auto value = _message_data->get_value("sig_type");
		if (value)
		{
			std::stringstream s;
			s << "type: ";
			s << value->get_as_string();

			bs.write<std::string>(s.str());
			bs.write<std::string>("\r\n");
		}
	}
//     if (_message_data)
//     {
//         bs.write<std::string>(_message_data->log());
//     }
    get_network_log().write_data(bs);

    //__LOG(core::log::info("core", boost::format("post message to gui, message=%1%\nparameters: %2%") % _message % (_message_data ? _message_data->log() : ""));)

    if (!gui_connector_)
    {
        assert(!"gui unlinked");
        return;
    }

    gui_connector_->receive(_message, _seq, _message_data);
}


icollection* core::core_dispatcher::create_collection()
{
    if (!core_factory_)
    {
        assert(!"core factory empty");
        return nullptr;
    }

    return core_factory_->create_collection();
}


void core_dispatcher::load_gui_settings()
{
    gui_settings_ = std::make_shared<core::gui_settings>(
        utils::get_product_data_path() + L"/settings/" + tools::from_utf8(gui_settings_file_name),
        utils::get_product_data_path() + L"/settings/" + tools::from_utf8(settings_export_file_name));

    gui_settings_->load();
    gui_settings_->start_save();
}

void core_dispatcher::load_hosts_config()
{
    hosts_config_ = std::make_shared<core::hosts_config>();

    hosts_config_->load();
    hosts_config_->start_save();
}

bool core_dispatcher::is_stats_enabled() const
{
    return true;
}

void core_dispatcher::load_statistics()
{
    if (!is_stats_enabled())
        return;

    statistics_ = std::make_shared<core::stats::statistics>(utils::get_product_data_path() + L"/stats/stats.stg");

    auto wr_stats = std::weak_ptr<core::stats::statistics>(statistics_);
    execute_core_context([this]
    {
        statistics_->init();
        start_session_stats(false /* delayed */);

        uint32_t timeout = (build::is_debug() ? (10 * 1000) : (5 * 60 * 1000));
        delayed_stat_timer_id_ = add_timer([this]
        {
            if (statistics_)
            {
                start_session_stats(true /* delayed */);
            }

            stop_timer(delayed_stat_timer_id_);
        }, timeout);
    });
}

void core_dispatcher::start_session_stats(bool _delayed)
{
    core::stats::event_props_type props;

    std::string uin = "";
    auto im = im_container_->get_im_by_id(1);
    if (im)
    {
        uin = g_core->get_root_login();
    }

    auto user_key = uin + g_core->get_uniq_device_id();
    auto hashed_user_key = core::tools::md5(user_key.c_str(), (int32_t)user_key.length());
    props.emplace_back(std::make_pair("hashed_user_key", hashed_user_key));
    props.emplace_back(std::make_pair("Sys_RAM", std::to_string(core::tools::system::get_memory_size_mb())));

    std::locale loc("");
    props.emplace_back(std::make_pair("Sys_Language", loc.name()));
    props.emplace_back(std::make_pair("Sys_OS_Version", core::tools::system::get_os_version_string()));

    if (voip_manager_ && voip_manager_->get_device_manager())
        props.emplace_back(std::make_pair("Sys_Video_Capture", std::to_string(voip_manager_->get_device_manager()->get_devices_number(voip2::DeviceType::VideoCapturing))));

    insert_event(_delayed ? core::stats::stats_event_names::start_session_params_loaded
        : core::stats::stats_event_names::start_session, props);
}

void core_dispatcher::insert_event(core::stats::stats_event_names _event)
{
    core::stats::event_props_type props;
    insert_event(_event, props);
}

void core_dispatcher::insert_event(core::stats::stats_event_names _event, const core::stats::event_props_type& _props)
{
    if (!is_stats_enabled())
        return;

    auto wr_stats = std::weak_ptr<core::stats::statistics>(statistics_);

    g_core->execute_core_context([wr_stats, _event, _props]
    {
        auto ptr_stats = wr_stats.lock();
        if (!ptr_stats)
            return;

        ptr_stats->insert_event(_event, _props);
    });
}

void core_dispatcher::load_theme_settings()
{
    theme_settings_ = std::make_shared<core::theme_settings>(utils::get_product_data_path() + L"/settings/theme.stg");
    theme_settings_->load();
    theme_settings_->start_save();
}

void core_dispatcher::post_gui_settings()
{
    coll_helper cl_coll(create_collection(), true);
    gui_settings_->serialize(cl_coll);

    cl_coll.set_value_as_string("data_path", core::tools::from_utf16(core::utils::get_product_data_path()));
    cl_coll.set_value_as_string("os_version", core::tools::system::get_os_version_string());


    post_message_to_gui("gui_settings", 0, cl_coll.get());
}

void core_dispatcher::post_logins()
{
    coll_helper cl_coll(create_collection(), true);
    gui_settings_->serialize(cl_coll);

    cl_coll.set_value_as_bool("has_valid_login", im_container_->has_valid_login());

    post_message_to_gui("core/logins", 0, cl_coll.get());
}

void core_dispatcher::post_theme_settings()
{
    coll_helper cl_coll(create_collection(), true);

    theme_settings_->serialize(cl_coll);

    post_message_to_gui("theme_settings", 0, cl_coll.get());
}

void core_dispatcher::post_need_promo()
{
    coll_helper cl_coll(create_collection(), true);
    cl_coll.set_value_as_bool("need_promo", settings_->get_need_show_promo());
    post_message_to_gui("need_show_promo_loaded", 0, cl_coll.get());
}

void core_dispatcher::post_app_config()
{
    coll_helper cl_coll(create_collection(), true);

    configuration::get_app_config().serialize(Out cl_coll);

    post_message_to_gui("app_config", 0, cl_coll.get());
}

void core::core_dispatcher::on_message_update_gui_settings_value(int64_t _seq, coll_helper _params)
{
    std::string value_name = _params.get_value_as_string("name");
    istream* value_data = _params.get_value_as_stream("value");

    tools::binary_stream bs_data;
    auto size = value_data->size();
    if (size)
        bs_data.write((const char*) value_data->read(size), size);

    gui_settings_->set_value(value_name, bs_data);
}

void core::core_dispatcher::on_message_update_theme_settings_value(int64_t _seq, coll_helper _params)
{
    std::string value_name = _params.get_value_as_string("name");
    istream* value_data = _params.get_value_as_stream("value");

    tools::binary_stream bs_data;
    auto size = value_data->size();
    if (size)
        bs_data.write((const char*) value_data->read(size), size);

    theme_settings_->set_value(value_name, bs_data);
    theme_settings_->save_if_needed();
}

void core::core_dispatcher::on_message_set_default_theme_id(int64_t _seq, coll_helper _params)
{
    const auto theme_id = _params.get_value_as_int("id");
    auto im = im_container_->get_im_by_id(1);
    const themes::theme *theme = im->get_theme_from_cache(theme_id);
    if (theme)
    {
        theme_settings_->set_value("default_theme", *theme);
    }
    else
    {
        theme_settings_->clear_values();
    }
    theme_settings_->save_if_needed();
}

void core::core_dispatcher::save_themes_etag(const std::string &etag)
{
    settings_->set_value(core_settings_values::themes_settings_etag, etag);
    settings_->save();
}

std::string core::core_dispatcher::load_themes_etag()
{
    return settings_->get_value(core_settings_values::themes_settings_etag, std::string(""));
}

void core::core_dispatcher::on_message_log(coll_helper _params) const
{
    const auto type = _params.get_value_as_string("type");
    const auto area = _params.get_value_as_string("area");
    const auto text = _params.get_value_as_string("text");

    if (!::strcmp(type, "trace"))
    {
        log::trace(area, text);
        return;
    }

    if (!::strcmp(type, "info"))
    {
        log::info(area, text);
        return;
    }

    if (!::strcmp(type, "warn"))
    {
        log::warn(area, text);
        return;
    }

    if (!::strcmp(type, "error"))
    {
        log::error(area, text);
        return;
    }

    assert(!"unknown log record type");
}

void core::core_dispatcher::on_message_profiler_proc_start(coll_helper _params) const
{
    const auto name = _params.get_value_as_string("name");
    const auto id = _params.get_value_as_int64("id");
    const auto ts = _params.get_value_as_int64("ts");

    profiler::process_started(name, id, ts);
}

void core::core_dispatcher::on_message_profiler_proc_stop(coll_helper _params) const
{
    const auto id = _params.get_value_as_int64("id");
    const auto ts = _params.get_value_as_int64("ts");

    profiler::process_stopped(id, ts);
}

void core::core_dispatcher::receive_message_from_gui(const char * _message, int64_t _seq, icollection* _message_data)
{
    // called from main thread
    std::string message_string = _message;

//     __LOG(
//         if (message_string != "log")
//         {
//             core::log::info("core", boost::format("message from gui, message=%1%\nparameters: %2%") % _message % (_message_data ? _message_data->log() : ""));
//         })

    if (_message_data)
        _message_data->addref();

    if (message_string == "history_search")
    {
        begin_search();
        begin_history_search();
    }

    execute_core_context([this, message_string, _seq, _message_data]
    {
        coll_helper params(_message_data, true);
        if (message_string != "log")
        {
            tools::binary_stream bs;
            bs.write<std::string>("GUI->CORE: message=");
            bs.write<std::string>(message_string);
            bs.write<std::string>("\r\n");
            if (message_string == "archive/messages/get")
            {
                std::stringstream s;
                s << "for: ";
                s << params.get_value_as_string("contact");
                s << " from: ";
                s << params.get_value_as_int64("from");
                s << " count_early: ";
                s << params.get_value_as_int64("count_early");
                s << " count_later: ";
                s << params.get_value_as_int64("count_later");

                bs.write<std::string>(s.str());
                bs.write<std::string>("\r\n");
            }
			// Added type of voip call to log.
			if (message_string == "voip_call")
			{
				std::stringstream s;
				s << "type: ";
				s << params.get_value_as_string("type");

				bs.write<std::string>(s.str());
				bs.write<std::string>("\r\n");
			}
        /*    if (_message_data)
            {
                bs.write<std::string>(_message_data->log());
            }*/
            get_network_log().write_data(bs);
        }

        if (message_string == "settings/value/set")
        {
            on_message_update_gui_settings_value(_seq, params);
        }
        else if (message_string == "log")
        {
            on_message_log(params);
        }
        else if (message_string == "profiler/proc/start")
        {
            on_message_profiler_proc_start(params);
        }
        else if (message_string == "profiler/proc/stop")
        {
            on_message_profiler_proc_stop(params);
        }
        else if (message_string == "themes/settings/set")
        {
            on_message_update_theme_settings_value(_seq, params);
        }
        else if (message_string == "themes/default/id")
        {
            on_message_set_default_theme_id(_seq, params);
        }
        else
        {
            im_container_->on_message_from_gui(message_string.c_str(), _seq, params);
        }
    });
}

const common::core_gui_settings& core::core_dispatcher::get_core_gui_settings() const
{
    return core_gui_settings_;
}

std::string core::core_dispatcher::get_root_login()
{
    auto im = im_container_->get_im_by_id(1);
    if (!im)
    {
        assert(false);
        return "";
    }

    return im->get_login();
}

std::string core::core_dispatcher::get_login_after_start()
{
    if (!im_container_)
    {
        assert(false);
        return "";
    }
    return im_container_->get_first_login();
}

std::string core::core_dispatcher::get_contact_friendly_name(unsigned _id, const std::string& _contact_login) {
    assert(!!im_container_);
    if (!!im_container_) {
        auto im = im_container_->get_im_by_id(_id);

        assert(!!im);
        if (!!im) {
            return im->get_contact_friendly_name(_contact_login);
        }
    }

    return "";
}

bool core::core_dispatcher::is_valid_search()
{
    return search_count_.load() == 1;
}

void core::core_dispatcher::begin_search()
{
    ++search_count_;
}

unsigned core::core_dispatcher::end_search()
{
    return --search_count_;
}

bool core::core_dispatcher::is_valid_history_search()
{
    return history_search_count_.load() == 1;
}

void core::core_dispatcher::begin_history_search()
{
    ++history_search_count_;
}

unsigned core::core_dispatcher::end_history_search()
{
    return --history_search_count_;
}

void core::core_dispatcher::update_login(im_login_id& _login)
{
    // if created new login
    if (!im_container_->update_login(_login))
    {
        g_core->post_message_to_gui("login_new_user", 0, nullptr);
    }

    start_session_stats(false /* delayed */);
}

void core::core_dispatcher::replace_uin_in_login(im_login_id& old_login, im_login_id& new_login)
{
    im_container_->replace_uin_in_login(old_login, new_login);
}

void core::core_dispatcher::post_voip_message(unsigned _id, const voip_manager::VoipProtoMsg& msg) {
    execute_core_context([this, _id, msg] {
        auto im = im_container_->get_im_by_id(_id);
        assert(!!im);

        if (!!im) {
            im->post_voip_msg_to_server(msg);
        }
    });
}

void core::core_dispatcher::post_voip_alloc(unsigned _id, const char* _data, size_t _len) {
    std::string data_str(_data, _len);
    execute_core_context([this, _id, data_str] {
        auto im = im_container_->get_im_by_id(_id);
        assert(!!im);

        if (!!im) {
            im->post_voip_alloc_to_server(data_str);
        }
    });
}

void core::core_dispatcher::on_thread_finish()
{
}

void core::core_dispatcher::unlogin(const bool _is_auth_error)
{
    execute_core_context([this, _is_auth_error]()
    {
        im_container_->logout(_is_auth_error);
    });
}

network_log& core::core_dispatcher::get_network_log()
{
    return (*network_log_);
}

proxy_settings core_dispatcher::get_proxy_settings()
{
    return proxy_settings_manager_->get_current_settings();
}

bool core_dispatcher::try_to_apply_alternative_settings()
{
    return proxy_settings_manager_->try_to_apply_alternative_settings();
}

void core_dispatcher::set_user_proxy_settings(const proxy_settings& _user_proxy_settings)
{
    proxy_settings_manager_->apply(_user_proxy_settings);
    g_core->post_user_proxy_to_gui();
}

bool core_dispatcher::locale_was_changed() const
{
    static bool stored = settings_->get_locale_was_changed();
    if (stored)
        settings_->set_locale_was_changed(false);
    return stored;
}

void core_dispatcher::set_locale(const std::string& _locale)
{
    static std::string stored;
    if (stored.empty())
        stored = get_locale();
    if (!stored.empty())
    {
        locale_was_changed(); // just store initiated value
        settings_->set_locale_was_changed(_locale != stored);
    }
    settings_->set_locale(_locale);
}

std::string core_dispatcher::get_locale() const
{
    return settings_->get_locale();
}

void core_dispatcher::post_user_proxy_to_gui()
{
    auto user_proxy = proxy_settings_manager_->get_current_settings();
    coll_helper cl_coll(create_collection(), true);
    user_proxy.serialize(cl_coll);
    g_core->post_message_to_gui("user_proxy/result", 0, cl_coll.get());
}

std::thread::id core_dispatcher::get_core_thread_id() const
{
   return core_thread_->get_core_thread_id();
}

bool core_dispatcher::is_core_thread() const
{
    return (std::this_thread::get_id() == get_core_thread_id());
}

void core_dispatcher::set_voip_mute_fix_flag(bool bValue)
{
	settings_->set_voip_mute_fix_flag(bValue);
}

bool core_dispatcher::get_voip_mute_fix_flag()
{
	return settings_->get_voip_mute_fix_flag();
}

hosts_config& core_dispatcher::get_hosts_config()
{
    assert(hosts_config_);

    return *hosts_config_;
}

void core_dispatcher::set_need_show_promo(bool _need_show_promo)
{
    settings_->set_need_show_promo(_need_show_promo);
}
