#ifndef __COLLECTION_H_
#define __COLLECTION_H_

#pragma once

#include "core_face.h"

#include "../core/tools/binary_stream.h"


namespace core
{
    class collection_value : public ivalue
    {
        collection_value_type type_;
        mutable char* log_data_;

        union data
        {
            char*				string_value_;
            int32_t				int_value_;
            int64_t				int64_value_;
            double				double_value_;
            bool				bool_value_;
            icollection*		collection_value_;
            iarray*				array_value_;
            istream*			istream_value_;
            ihheaders_list*		ihheaders_value_;
            uint32_t			uint_value_;

        } data__;

        // ibase interface
        std::atomic<int32_t>		ref_count_;
        virtual int32_t addref() override;
        virtual int32_t release() override;

        // ivalue interface
        virtual void set_as_int(int32_t) override;
        virtual int32_t get_as_int() override;

        virtual void set_as_int64(int64_t) override;
        virtual int64_t get_as_int64() const override;

        virtual void set_as_string(const char*, int32_t len) override;
        virtual const char* get_as_string() const override;

        virtual void set_as_double(double) override;
        virtual double get_as_double() override;

        virtual void set_as_bool(bool) override;
        virtual bool get_as_bool() override;

        virtual void set_as_collection(icollection*) override;
        virtual icollection* get_as_collection() const override;

        virtual void set_as_stream(istream*) override;
        virtual istream* get_as_stream() override;

        virtual void set_as_array(iarray*) override;
        virtual iarray* get_as_array() override;

        virtual void set_as_hheaders(ihheaders_list*) override;
        virtual ihheaders_list* get_as_hheaders() override;

        virtual void set_as_uint(uint32_t) override;
        virtual uint32_t get_as_uint() override;

        virtual const char* log() const override;

        void clear();

    public:

        collection_value();
        virtual ~collection_value();
    };

    class coll_array : public core::iarray
    {
        std::vector<ivalue*>		vec_;

        // ibase interface
        std::atomic<int32_t>		ref_count_;
        virtual int32_t addref() override;
        virtual int32_t release() override;

        virtual void push_back(ivalue*) override;
        virtual const ivalue* get_at(int32_t) const override;
        virtual void reserve(int32_t) override;
        virtual int32_t size() const override;
        virtual bool empty() const override;
    public:
        coll_array();
        virtual ~coll_array();
    };

    class coll_stream : public core::istream
    {
        // ibase interface
        std::atomic<int32_t> ref_count_;
        virtual int32_t addref() override;
        virtual int32_t release() override;

        core::tools::binary_stream	stream_;

        virtual uint8_t* read(uint32_t _size) override;
        virtual void write(std::istream& _source) override;
        virtual void write(const uint8_t* _buffer, uint32_t _size) override;
        virtual bool empty() const override;
        virtual uint32_t size() const override;
        virtual void reset() override;

    public:
        coll_stream();
        virtual ~coll_stream();
    };

    class hheaders_list : public ihheaders_list
    {
        std::list<hheader*>				headers_;
        std::list<hheader*>::iterator	cursor_;

        std::atomic<int32_t>	ref_count_;
        virtual int32_t addref() override;
        virtual int32_t release() override;

    public:

        virtual void push_back(hheader*) override;
        virtual hheader* first() override;
        virtual hheader* next() override;
        virtual int32_t count() override;
        virtual bool empty() override;

        hheaders_list();
        virtual ~hheaders_list();
    };

    class collection : public core::icollection
    {
        std::atomic<int32_t> ref_count_;
        std::map<std::string, core::ivalue*> values_;
        std::map<std::string, core::ivalue*>::iterator cursor_;

        mutable char* log_data_;

        void clear();

        // ibase interface
        virtual int32_t addref() override;
        virtual int32_t release() override;

        virtual ivalue* create_value() override;
        virtual icollection* create_collection() override;
        virtual iarray* create_array() override;
        virtual istream* create_stream() override;
        virtual ihheaders_list* create_hheaders_list() override;

        virtual void set_value(const char* name, ivalue* value) override;
        virtual ivalue* get_value(const char* name) const override;

        virtual ivalue* first() override;
        virtual ivalue* next() override;
        virtual int32_t count() override;
        virtual bool empty() override;
        virtual bool is_value_exist(const char* name) const override;
        virtual const char* log() const override;
    public:

        collection();
        virtual ~collection();
    };


}


#endif//__COLLECTION_H_
