#pragma once

#include "../../../namespaces.h"

enum class loader_errors;

CORE_NS_BEGIN

class async_executer;

struct proxy_settings;

CORE_NS_END

CORE_WIM_NS_BEGIN

enum class tasks_runner_slot;

struct wim_packet_params;

class loader_task
{
public:
    virtual ~loader_task() = 0;

    virtual void cancel() = 0;

    virtual const std::string& get_contact_aimid() const = 0;

    virtual int64_t get_id() const = 0;

    virtual tasks_runner_slot get_slot() const = 0;

    virtual void on_result(const loader_errors _error) = 0;

    virtual void on_before_resume(
        const wim_packet_params &_wim_params,
        const proxy_settings &_proxy_settings,
        const bool _is_genuine) = 0;

    virtual void on_before_suspend() = 0;

    virtual loader_errors run() = 0;

    virtual std::string to_log_str() const = 0;

};

CORE_WIM_NS_END