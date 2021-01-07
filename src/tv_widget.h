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
	TVWidget(const Glib::ustring& itemName, const Glib::ustring& imgPath, const Glib::ustring& default_path);
	virtual ~TVWidget();

	void init(const Glib::ustring& itemName, const Glib::ustring& imgPath, const Glib::ustring& default_path);
	void SetupTorrents();
	void update();
	void notify();

	inline Glib::ustring GetName() const { return m_Item.name;  }

	inline Glib::ustring GetImgPath() const { return m_Item.img_path; }

	inline Gtk::Box& GetBox() { return *m_Box; }

	inline TVItem& GetItem() { return m_Item; }
	inline TorrentHandler& GetHandler() { return m_Handler; }

private:
	
	Glib::RefPtr<Gtk::Builder> builder;

	Gtk::Box* m_Box{};
	Gtk::Box* m_DownloadStatus{};
	Gtk::Label* m_Downloads{};
	Gtk::Label* FileName{};
	Gtk::Label* UL_Speed{};
	Gtk::Label* DL_Speed{};
	Gtk::ProgressBar* Progress{};

	TVItem m_Item;

	TorrentHandler m_Handler;

	std::thread* first{};
	Glib::Dispatcher m_Dispatcher;

	const int desiredWidth = 180;
	const int desiredHeight = 480;

	int subscription{};
};


#endif
