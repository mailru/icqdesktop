#ifndef __TYPEDEFS_H_
#define __TYPEDEFS_H_

namespace core
{
	typedef std::map<std::string, std::string> str_2_str_map;

	typedef std::set<int32_t> int_set;

	typedef std::set<std::string> str_set;

    typedef std::vector<std::string> string_vector_t;

    typedef std::shared_ptr<string_vector_t> string_vector_sptr_t;

    typedef std::atomic<bool> atomic_bool;
}

#endif // __TYPEDEFS_H_