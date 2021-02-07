#include "tv_widget.h"
#include "gtkmm/textbuffer.h"
#include "macros.h"
#include "formatter.h"
#include "resource_manager.h"
#include "container.h"

TVWidget::TVWidget(const Glib::ustring& itemName, const Glib::ustring& imgPath)
	:m_Dispatcher(),
	hash(Unique::from_string(itemName))
{
	init(itemName, imgPath);
}

void TVWidget::init(const Glib::ustring &itemName, const Glib::ustring &imgPath) {

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
	if(!imgPath.empty()) {
	    auto pixbuf = Gdk::Pixbuf::create_from_file(imgPath, desiredWidth, desiredHeight);
	    m_Image->set(pixbuf);
	}
	m_Dispatcher.ON_DISPATCH(&TVWidget::update);
	//first = new std::thread([this] { m_Handler.do_work(); });

	subscription = DataContainer::get_group(hash).second->subscribe((const std::function<void()> &) [this] { TVWidget::notify(); });
}

void TVWidget::update() {
    auto group = DataContainer::get_group(hash);
	m_Downloads->set_label(Glib::ustring::format(group.second->m_Handles.size(), " downloads"));
	
	lt::torrent_status maxi;
	for(auto& pair : group.second->m_Handles) {
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

	DataContainer::get_group(hash).second->unsubscribe(subscription);
	delete m_Box;
}
