#ifndef TVTORRENT_ITEM_H
#define TVTORRENT_ITEM_H
#include <glibmm/ustring.h>
#include <vector>
#include "hash.h"

struct TVItem {

    Glib::ustring name;
	Glib::ustring img_path;
	Glib::ustring default_save_path;

	const size_t hash = Unique::from_string(name);
};
#endif
