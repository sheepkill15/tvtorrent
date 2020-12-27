#ifndef TVTORRENT_ITEM_H
#define TVTORRENT_ITEM_H
#include <glibmm/ustring.h>
#include <vector>

struct TVTorrent {
	Glib::ustring magnet_uri;
	Glib::ustring file_path;
};

struct TVItem {
	Glib::ustring name;
	Glib::ustring img_path;
	std::vector<TVTorrent> torrents;
};

#endif
