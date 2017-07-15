#pragma once

#include "ibase.h"

#include "ivalue.h"

#include "../common.shared/common_defs.h"

namespace core
{
	enum login_error
	{
		le_success			= 0,
		le_wrong_login		= 1,
		le_network_error	= 2,
		le_parse_response	= 3,
		le_rate_limit		= 4,
		le_unknown_error	= 5,
		le_invalid_sms_code = 6,
		le_error_validate_phone = 7,
        le_attach_error_busy_phone = 8,
        le_wrong_login_2x_factor = 9,
	};

	enum avatar_error
	{
		ae_success			= 0,
		ae_network_error	= 1,
		ae_unknown_error	= 2
	};

	enum collection_value_type
	{
		vt_empty,
		vt_int,
		vt_double,
		vt_string,
		vt_bool,
		vt_collection,
		vt_stream,
		vt_array,
		vt_int64,
		vt_hheaders,
		vt_uint
	};

	struct istream : ibase
	{
		virtual uint8_t* read(uint32_t) = 0;
        virtual void write(std::istream& _source) = 0;
		virtual void write(const uint8_t*, uint32_t) = 0;
		virtual bool empty() const = 0;
		virtual uint32_t size() const = 0;
		virtual void reset() = 0;
		virtual ~istream() {}
	};

	struct iarray : ibase
	{
		virtual void push_back(ivalue*) = 0;
		virtual const ivalue* get_at(int32_t) const = 0;
		virtual void reserve(int32_t) = 0;
		virtual int32_t size() const = 0;
		virtual bool empty() const = 0;
	};

	struct hheader
	{
		int64_t		id_;
		int64_t		prev_id_;
		uint64_t	time_;
	};

	struct ihheaders_list : ibase
	{
		virtual void push_back(hheader*) = 0;
		virtual hheader* first() = 0;
		virtual hheader* next() = 0;
		virtual int32_t count() = 0;
		virtual bool empty() = 0;
	};

    struct icollection : ibase
    {
        virtual bool is_value_exist(const char* name) const = 0;

        virtual ivalue* create_value() = 0;
        virtual icollection* create_collection() = 0;
        virtual iarray* create_array() = 0;
        virtual istream* create_stream() = 0;
        virtual ihheaders_list* create_hheaders_list() = 0;

        virtual void set_value(const char* name, ivalue* value) = 0;
        virtual ivalue* get_value(const char* name) const = 0;
        virtual ivalue* first() = 0;
        virtual ivalue* next() = 0;
        virtual int32_t count() = 0;
        virtual bool empty() = 0;

        virtual const char* log() const = 0;

        virtual ~icollection() {}
    };

	struct iconnector : ibase
	{
		virtual void link(iconnector*, const common::core_gui_settings&) = 0;
		virtual void unlink() = 0;
        virtual void receive(const char*, int64_t, core::icollection*) = 0;

		virtual ~iconnector() {}

	};

	struct icore_factory : ibase
	{
		virtual icollection* create_collection() = 0;
		~icore_factory() {}
	};

	struct icore_interface : ibase
	{
		virtual iconnector* get_core_connector() = 0;
		virtual iconnector* get_gui_connector() = 0;
		virtual icore_factory* get_factory() = 0;

		virtual ~icore_interface() {}
	};
}
