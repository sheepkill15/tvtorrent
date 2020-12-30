#include "main_window.h"
#include "gtkmm/dialog.h"
#include "gtkmm/entry.h"
#include "gtkmm/filechooser.h"
#include "gtkmm/filechooserbutton.h"
#include "gtkmm/filechooserdialog.h"
#include "gtkmm/filefilter.h"
#include "gtkmm/headerbar.h"
#include "gtkmm/hvbox.h"
#include "gtkmm/messagedialog.h"
#include "tv_widget.h"
#include <gtkmm/enums.h>
#include <gtkmm/builder.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/comboboxtext.h>
#include <iostream>
#include <memory>
#include "macros.h"
#include "item_window.h"
#include "resource_manager.h"

TTMainWindow::TTMainWindow()
	: tvw_list(),
	m_Feed("https://www.erai-raws.info/rss-1080")
{
	set_title("TVTorrent");
	set_border_width(10);
	set_default_size(640, 480);

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

	set_titlebar(*headerbar);

	init_items();

	add(m_VBox);

	m_VBox.pack_start(m_ScrolledWindow);

	m_ScrolledWindow.add(m_FlowBox);
	m_ScrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

	m_FlowBox.ON_BUTTON_PRESSED(&TTMainWindow::on_tvwidget_double_click);

	show_all_children();

}

TTMainWindow::~TTMainWindow() {

	ResourceManager::create_save(tvw_list);

	for(auto tvw : tvw_list) {
		delete tvw;
	}
}

void TTMainWindow::external_torrent(char argv[]) {
	std::cout << argv << std::endl;

	auto builder = Gtk::Builder::create_from_file(ResourceManager::get_resource_path("tvtorrent_torrentfile.glade"));
	Gtk::MessageDialog* dialog;
	builder->get_widget("TorrentFromFile", dialog);

	Gtk::ComboBoxText* item_list;
	builder->get_widget("ItemList", item_list);
	int i = 0;
	for(auto tvw : tvw_list) {
		item_list->insert(i, tvw->GetItem().name);
		i++;
	}

	item_list->set_active(0);

	Gtk::FileChooserButton* file_path;
	builder->get_widget("TorrentPath", file_path);

	int result = dialog->run();
	switch(result) {
		case Gtk::RESPONSE_YES:
		{
			std::cout << "Yes" << std::endl;
			auto tvw = tvw_list[item_list->get_active_row_number()];
			tvw->GetItem().torrents.push_back({argv, file_path->get_filename()});
			tvw->GetHandler().AddTorrent(argv, file_path->get_filename());
			break;
		}
		case Gtk::RESPONSE_NO:
			std::cout << "No" << std::endl;
			break;
		default:
			std::cout << "What" << std::endl;
			break;
	}
	dialog->hide();
	delete dialog;
}

void TTMainWindow::init_items() {
	Json::Value root;

	bool ok = ResourceManager::get_save(root);

	if(ok) {
		for(int i = 0; i < root.size(); i++) {
			add_item(root[i]["name"].asString(), root[i]["img_path"].asString());
			if(root[i].isMember("torrents") && root[i]["torrents"]) {
				for(int j = 0; j < root[i]["torrents"].size(); j++) {
					tvw_list[i]->GetItem().torrents.push_back({root[i]["torrents"][j]["magnet_uri"].asString(), root[i]["torrents"][j]["file_path"].asString()});
				}
				tvw_list.back()->SetupTorrents();
			}
		}
	}
}

void TTMainWindow::add_item(const Glib::ustring& name, const Glib::ustring& img_path) {

	if(name.empty() || img_path.empty()) return;

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

	int result = dialog->run();
	switch(result) {
		case(Gtk::RESPONSE_OK):
			std::cout << "OK" << std::endl;
			add_item(tv->get_text(), file_name->get_filename());
			break;
		case(Gtk::RESPONSE_CANCEL):
			std::cout << "Cancel" << std::endl;
			break;
		default:
			std::cout << "What" << std::endl;
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
	delete tvw;
}

void TTMainWindow::on_button_settings() {
	
}

bool TTMainWindow::on_tvwidget_double_click(GdkEventButton* ev) {
	
	if(ev->type == GDK_2BUTTON_PRESS) {
		std::cout << "Double click!" << std::endl;	
		auto selected = m_FlowBox.get_selected_children();
		auto window = new TTItemWindow(*tvw_list[selected[0]->get_index()]);
		window->ON_HIDE_BIND(&TTMainWindow::on_item_window_hide, TTItemWindow*), window));
		window->show();
		//get_application()->add_window(*window);
	} 

	return true;
}

void TTMainWindow::on_item_window_hide(TTItemWindow* window) {
	delete window;
}

