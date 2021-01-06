#include "tv_widget.h"
#include "gtkmm/textbuffer.h"
#include <iostream>
#include "macros.h"
#include "formatter.h"
#include "resource_manager.h"

TVWidget::TVWidget(const Glib::ustring& itemName, const Glib::ustring& imgPath, const Glib::ustring& default_path)
	:m_Dispatcher()
{
	init(itemName, imgPath, default_path);
}

void TVWidget::SetupTorrents() {
	for(auto& tvt : m_Item.torrents) {
		m_Handler.AddTorrent(tvt.magnet_uri, tvt.file_path);
	}
	update();
}

void TVWidget::init(const Glib::ustring &itemName, const Glib::ustring &imgPath, const Glib::ustring& default_path) {
	m_Item = {itemName, imgPath, default_path};

	builder = Gtk::Builder::create_from_file(ResourceManager::get_resource_path("tvtorrent_tvwidget.glade"));
	builder->get_widget("TVWidget", m_Box);
	Gtk::Image* m_Image;
	Gtk::Label* m_Name;
	builder->get_widget("TVW_Image", m_Image);
	builder->get_widget("TVW_Name", m_Name);
	builder->get_widget("TVW_Downloads", m_Downloads);
	builder->get_widget("DownloadStatus", m_DownloadStatus);
	builder->get_widget("UL_Speed", UL_Speed);
	builder->get_widget("DL_Speed", DL_Speed);
	builder->get_widget("Progress", Progress);
	builder->get_widget("FileName", FileName);
	
	m_Name->set_label(itemName);
	auto pixbuf = Gdk::Pixbuf::create_from_file(imgPath, desiredWidth, desiredHeight);
	m_Image->set(pixbuf);
	m_Dispatcher.ON_DISPATCH(&TVWidget::update);
	first = new std::thread([this] { m_Handler.do_work(); });

	subscription = m_Handler.subscribe((const std::function<void()> &) [this] { TVWidget::notify(); });
}

void TVWidget::update() {
	m_Downloads->set_label(Glib::ustring::format(m_Item.torrents.size(), " downloads"));
	
	lt::torrent_status maxi;
	for(auto& pair : m_Handler.m_Handles) {
		auto& handle = pair.second;
		if(!handle.is_valid()) continue;
		auto status = handle.status();
		if(status.state == lt::torrent_status::downloading && status.progress_ppm > maxi.progress_ppm) {
			maxi = status;
		}
	}
	if(maxi.state == lt::torrent_status::downloading) {
		FileName->set_label(maxi.name);
		Progress->set_fraction(maxi.progress);
		DL_Speed->set_label(Formatter::format_size(maxi.download_payload_rate) + "/s");
		UL_Speed->set_label(Formatter::format_size(maxi.upload_payload_rate) + "/s");
	}
	else {
		FileName->set_label("No file is downloading right now");
		Progress->set_fraction(0);
		DL_Speed->set_label(Formatter::format_size(0) + "/s");
		UL_Speed->set_label(Formatter::format_size(0) + "/s");
	}

}

void TVWidget::notify() {
	m_Dispatcher.emit();
}

TVWidget::~TVWidget() {

	m_Handler.unsubscribe(subscription);

	delete m_Box;
	m_Handler.signal_stop();
	first->detach();
	delete first;
}
