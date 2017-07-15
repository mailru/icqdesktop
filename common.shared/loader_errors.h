#pragma once

enum class loader_errors
{
    undefined               = -1,

    success					= 0,

    file_not_exits			= 101,
    open_file_error			= 102,
    get_gateway_error		= 103,
    max_file_size_error		= 104,
    empty_file				= 105,
    read_from_file			= 106,
    send_range				= 107,
    invalid_url				= 108,
    get_metainfo			= 109,
    create_file				= 110,
    create_directory		= 111,
    load_range				= 112,

    network_error			= 113,
    save_2_file				= 114,
    move_file				= 115,
    store_info				= 116,
    too_large_file          = 117,
    invalid_json            = 118,
    http_error              = 119,
    orphaned                = 120,
    suspend                 = 121,
    cancelled               = 122,
    no_link_preview         = 123,
    metainfo_not_found      = 124,

    internal_logic_error	= 201
};
