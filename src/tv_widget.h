#ifndef TVTORRENT_TV_WIDGET_H
#define TVTORRENT_TV_WIDGET_H

#include "glibmm/dispatcher.h"
#include "glibmm/ustring.h"
#include "gtkmm/builder.h"
#include "gtkmm/container.h"
#include "gtkmm/enums.h"
#include "gtkmm/hvbox.h"
#include "gtkmm/image.h"
#include "gtkmm/label.h"
#include <gtkmm/progressbar.h>
#include "torrent_handler.h"
#include "tv_item.h"
#include "feed.h"

class TVWidget {
public:
	
	TVWidget() = default;
	TVWidget(const Glib::ustring& itemName, const Glib::ustring& imgPath);
	virtual ~TVWidget();

	void init(const Glib::ustring& itemName, const Glib::ustring& imgPath);
	void update();
	void notify();

	inline Gtk::Box& GetBox() { return *m_Box; }

    const size_t hash;
private:
	
	Glib::RefPtr<Gtk::Builder> builder;

	Gtk::Box* m_Box{};
	Gtk::Box* m_DownloadStatus{};
	Gtk::Label* m_Downloads{};
	Gtk::Label* FileName{};
	Gtk::Label* UL_Speed{};
	Gtk::Label* DL_Speed{};
	Gtk::ProgressBar* Progress{};

	Glib::Dispatcher m_Dispatcher;

	const int desiredWidth = 180;
	const int desiredHeight = 480;

    int subscription{};
};


#endif
