#include "main_window.h"
#include "gtkmm/filechooser.h"
#include "gtkmm/filechooserbutton.h"
#include <gtkmm/toolbar.h>
#include <gtkmm/comboboxtext.h>
#include <memory>
#include <utility>
#include "macros.h"
#include "resource_manager.h"
#include "feed_control_window.h"
#include "logger.h"
#include "container.h"
#include "settings_manager.h"

TTMainWindow::TTMainWindow()
	: tvw_list(),
	m_Dispatcher()
{
	set_title("TVTorrent");
	set_border_width(10);
	set_default_size(1280, 720);

	m_refBuilder = Gtk::Builder::create_from_file(ResourceManager::get_resource_path("tvtorrent_headerbar.glade"));
	Gtk::HeaderBar* headerbar = nullptr;
	m_refBuilder->get_widget("HeaderBar", headerbar);

	Gtk::Button* add_button = nullptr;
	m_refBuilder->get_widget("AddButton", add_button);
	add_button->ON_CLICK(&TTMainWindow::on_button_add);

	Gtk::Button* remove_button = nullptr;
	m_refBuilder->get_widget("RemoveButton", remove_button);
	remove_button->ON_CLICK(&TTMainWindow::on_button_remove);

	Gtk::Button* settings_button = nullptr;
	m_refBuilder->get_widget("SettingsButton", settings_button);
	settings_button->ON_CLICK(&TTMainWindow::on_button_settings);

	Gtk::Button* feeds_button = nullptr;
	m_refBuilder->get_widget("FeedsButton", feeds_button);
	feeds_button->ON_CLICK(&TTMainWindow::on_button_feeds);

	set_titlebar(*headerbar);

	init_items();
	add(m_VBox);

	m_VBox.pack_start(m_ScrolledWindow);

	m_ScrolledWindow.add(m_FlowBox);
	m_ScrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

	m_FlowBox.ON_BUTTON_PRESSED(&TTMainWindow::on_tvwidget_double_click);

	show_all_children();
	m_Dispatcher.ON_DISPATCH(&TTMainWindow::external_torrent_empty);
	just_show_please.ON_DISPATCH(&TTMainWindow::show);
}

TTMainWindow::~TTMainWindow() {

    for(auto tvw : tvw_list) {
        delete tvw;
    }

}

void TTMainWindow::external_torrent(char argv[]) {
	auto builder = Gtk::Builder::create_from_file(ResourceManager::get_resource_path("tvtorrent_torrentfile.glade"));
	Gtk::MessageDialog* dialog;
	builder->get_widget("TorrentFromFile", dialog);

	Gtk::ComboBoxText* item_list;
	builder->get_widget("ItemList", item_list);
	int i = 0;
	for(auto& pair : DataContainer::get_groups()) {
		item_list->insert(i, pair.first->name);
		i++;
	}

	item_list->set_active(0);

	Gtk::FileChooserButton* file_path;
	builder->get_widget("TorrentPath", file_path);

	int result = dialog->run();
	switch(result) {
		case Gtk::RESPONSE_YES:
		{
			auto pair = DataContainer::get_group(item_list->get_active_text());
			pair.first->torrents.push_back({argv, file_path->get_filename()});
			pair.second->AddTorrent(argv, file_path->get_filename());
			break;
		}
	    case Gtk::RESPONSE_NO:
		default:
			break;
	}
	dialog->hide();
	delete dialog;
}

void TTMainWindow::add_item(const Glib::ustring& name, const Glib::ustring& img_path, const Glib::ustring& default_path) {

	if(name.empty() || default_path.empty()) return;
	DataContainer::add_group(name, img_path, default_path);

	tvw_list.push_back(new TVWidget(name, img_path));
	m_FlowBox.insert(tvw_list.back()->GetBox(), tvw_list.size() - 1);

	show_all_children();
}

void TTMainWindow::on_button_add() {

	Gtk::Dialog* dialog;
	auto builder = Gtk::Builder::create_from_file(ResourceManager::get_resource_path("tvtorrent_additem.glade"));
	builder->get_widget("ItemDialog", dialog);

	Gtk::FileChooserButton* file_name;
	builder->get_widget("ImageFile", file_name);

	Gtk::Entry* tv;
	builder->get_widget("ItemName", tv);

	Gtk::FileChooserButton* default_path;
	builder->get_widget("DefaultPath", default_path);

	int result = dialog->run();
	switch(result) {
		case(Gtk::RESPONSE_OK):
			add_item(tv->get_text(), file_name->get_filename(), default_path->get_filename());
			break;
		case(Gtk::RESPONSE_CANCEL):
		default:
			break;
	}
	dialog->hide();
	delete dialog;
}

void TTMainWindow::on_button_remove() {
	
	auto selected = m_FlowBox.get_selected_children();
	if(selected.empty()) return;
	int index = selected[0]->get_index();
	if(index < 0) return;
	auto tvw = tvw_list[index];
	m_FlowBox.remove(tvw->GetBox());
	tvw_list.erase(tvw_list.begin() + index);
    size_t hash = tvw->hash;
    delete tvw;
	DataContainer::remove_group(hash);

}

void TTMainWindow::on_button_settings() {
	if(settings_window != nullptr) {
	    settings_window->window->show();
	    return;
	}
	settings_window = new TTSettingsWindow(this);
	settings_window->window->ON_HIDE(&TTMainWindow::on_settings_window_hide);
}

bool TTMainWindow::on_tvwidget_double_click(GdkEventButton* ev) {
	
	if(ev->type == GDK_2BUTTON_PRESS) {
		auto selected = m_FlowBox.get_selected_children();
		if(selected.empty()) return false;
		auto sel = selected[0];
		if(!sel) return false;
		int index = sel->get_index();
		if(index < 0) return false;
		TVWidget* item = tvw_list[index];
		Logger::info("Creating item window");
		auto window = new TTItemWindow(item->hash);
		Logger::info("Created!");
		window->ON_HIDE_BIND(&TTMainWindow::on_item_window_hide, TTItemWindow*), window));
		window->show();
		//get_application()->add_window(*window);
	} 

	return true;
}

void TTMainWindow::on_item_window_hide(TTItemWindow* window) {
	delete window;
}

void TTMainWindow::on_button_feeds() {
    if(feed_control_window != nullptr) {
        feed_control_window->window->show();
        return;
    }
    feed_control_window = new TTFeedControlWindow();
    feed_control_window->window->ON_HIDE(&TTMainWindow::on_feedcontrol_window_hide);
    DataContainer::get_manager().pause();
}

void TTMainWindow::on_feedcontrol_window_hide() {
    delete feed_control_window;
    feed_control_window = nullptr;
    DataContainer::get_manager().resume();
    DataContainer::get_manager().refresh_check();
}
void TTMainWindow::on_settings_window_hide() {
    delete settings_window;
    settings_window = nullptr;
}

void TTMainWindow::update_limits() {
    for(auto group : DataContainer::get_groups()) {
        group.second->update_limits();
    }
}
void TTMainWindow::external_torrent_empty() {
    external_torrent(pending_uri.data());
}

void TTMainWindow::notify(const std::string & uri) {
    pending_uri = uri;
    m_Dispatcher.emit();
}

void TTMainWindow::init_items() {
    for(auto& group : DataContainer::get_groups()) {
        tvw_list.push_back(new TVWidget(group.first->name, group.first->img_path));
        m_FlowBox.insert(tvw_list.back()->GetBox(), tvw_list.size() - 1);
    }
}

