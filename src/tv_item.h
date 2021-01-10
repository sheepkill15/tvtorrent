#ifndef TVTORRENT_ITEM_H
#define TVTORRENT_ITEM_H
#include <glibmm/ustring.h>
#include <vector>
#include "hash.h"

struct TVTorrent {
	Glib::ustring magnet_uri;
	Glib::ustring file_path;

	size_t hash = Unique::from_string(magnet_uri);

};

struct TVItem {

    Glib::ustring name;
	Glib::ustring img_path;
	Glib::ustring default_save_path;
	std::vector<TVTorrent> torrents;

	const size_t hash = Unique::from_string(name);
};
#endif
