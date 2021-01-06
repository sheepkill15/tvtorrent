#include "giomm/simpleactiongroup.h"
#include "gtkmm/box.h"
#include "gtkmm/button.h"
#include "tv_widget.h"
#include "item_window.h"
#include "gtkmm/builder.h"
#include "gtkmm/dialog.h"
#include "gtkmm/entry.h"
#include "gtkmm/enums.h"
#include "gtkmm/messagedialog.h"
#include "gtkmm/object.h"
#include <cstdlib>
#include <filesystem>
#include <gtkmm/liststore.h>
#include <gtkmm/cellrendererprogress.h>
#include <gtkmm/headerbar.h>
#include <gtkmm/filechooserbutton.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/toolbar.h>
#include "macros.h"
#include "formatter.h"
#include "resource_manager.h"
#include <iostream>
#include <unistd.h>

TTItemWindow::TTItemWindow(TVWidget& item) 
	: m_Item(&item.GetItem()),
	m_Dispatcher(),
	m_TorrentHandler(item.GetHandler()),
	m_Box(Gtk::ORIENTATION_VERTICAL)
{
	set_title(m_Item->name);
	set_border_width(10);
	set_default_size(640, 480);
	
	auto m_refBuilder = Gtk::Builder::create_from_file(ResourceManager::get_resource_path("tvtorrent_headerbar.glade"));

	Gtk::HeaderBar* headerbar = nullptr;
	m_refBuilder->get_widget("HeaderBar", headerbar);
	headerbar->set_title(item.GetName());

	Gtk::Button* add_button = nullptr;
	m_refBuilder->get_widget("AddButton", add_button);
	add_button->ON_CLICK(&TTItemWindow::on_button_add);
	set_titlebar(*headerbar);

	Gtk::Button* remove_button = nullptr;
	m_refBuilder->get_widget("RemoveButton", remove_button);
	remove_button->ON_CLICK(&TTItemWindow::on_button_remove);

	Gtk::Button* settings_button = nullptr;
	m_refBuilder->get_widget("SettingsButton", settings_button);
	settings_button->ON_CLICK(&TTItemWindow::on_button_settings);

	builder = Gtk::Builder::create_from_file(ResourceManager::get_resource_path("tvtorrent_addtorrent.glade"));
	builder->get_widget<Gtk::MessageDialog>("AddTorrentDialog", m_Dialog);

	Gtk::FileChooserButton* path;
	builder->get_widget("FilePath", path);
	path->set_filename(m_Item->default_save_path);

	m_Dialog->ON_RESPONSE(&TTItemWindow::on_torrentadddialog_response);

	remove_builder = Gtk::Builder::create_from_file(ResourceManager::get_resource_path("tvtorrent_removetorrent.glade"));
	remove_builder->get_widget("RemoveTorrentDialog", m_RemoveDialog);
	m_RemoveDialog->ON_RESPONSE(&TTItemWindow::on_torrentremovedialog_response);

	add(m_Box);

	auto toolbar_builder = Gtk::Builder::create_from_file(ResourceManager::get_resource_path("tvtorrent_torrentcontrols.glade"));
	Gtk::Toolbar* toolbar; 
	toolbar_builder->get_widget("TorrentControls", toolbar);

	auto m_refActionGroup = Gio::SimpleActionGroup::create();
	m_refActionGroup->ADD_ACTION("start", &TTItemWindow::on_start_torrent);
	m_refActionGroup->ADD_ACTION("pause", &TTItemWindow::on_pause_torrent);

	insert_action_group("tvtorrent", m_refActionGroup);

	m_Box.pack_start(*toolbar, Gtk::PACK_SHRINK);

	m_Box.pack_start(m_ScrolledWindow, Gtk::PACK_EXPAND_WIDGET);
	m_ScrolledWindow.set_border_width(10);
	m_ScrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	m_ScrolledWindow.add(m_TreeView);	

	m_refTreeModel = Gtk::ListStore::create(m_Columns);
	m_TreeView.set_model(m_refTreeModel);

	m_TreeView.append_column("ID", m_Columns.m_col_id);
	m_TreeView.append_column("Name", m_Columns.m_col_name);
	m_TreeView.append_column("State", m_Columns.m_col_state);

	auto cell = Gtk::make_managed<Gtk::CellRendererProgress>();
	int cols_count = m_TreeView.append_column("Progress", *cell);
	auto pColumn = m_TreeView.get_column(cols_count - 1);
	if(pColumn) {
		pColumn->add_attribute(cell->property_value(), m_Columns.m_col_progress);
	}

	m_TreeView.append_column("Download", m_Columns.m_col_dl);
	m_TreeView.append_column("Upload", m_Columns.m_col_ul);
	m_TreeView.append_column("Size", m_Columns.m_col_size);
	m_TreeView.append_column("ETA", m_Columns.m_col_eta);

	for(guint i = 0; i < m_TreeView.get_n_columns(); i++) {
		auto column = m_TreeView.get_column(i);
		column->set_reorderable();
		column->set_resizable();
	}
	show_all_children();

	for(auto& pair : m_TorrentHandler.m_Handles) {
		add_torrent_row(pair.second);
	}

	m_TreeView.ON_BUTTON_PRESSED(&TTItemWindow::on_row_pressed);

	m_Dispatcher.ON_DISPATCH(&TTItemWindow::update_torrent_views);
	//first = new std::thread([this] { m_TorrentHandler.do_work(this); });
	subscription = m_TorrentHandler.subscribe((const std::function<void()> &) [this] { TTItemWindow::notify(); });
}

void TTItemWindow::notify() {
	m_Dispatcher.emit();
}

void TTItemWindow::update_torrent_views() {

	for(const auto& row: m_refTreeModel->children()) {
	    auto& handle = m_TorrentHandler.m_Handles[row->get_value(m_Columns.m_col_name)];
	    if(!handle.is_valid()) continue;
		auto status = handle.status();
		row[m_Columns.m_col_progress] = status.progress_ppm / 10000;
		row[m_Columns.m_col_state] = TorrentHandler::state(status);
		row[m_Columns.m_col_dl] = Formatter::format_size(status.download_payload_rate) + "/s";
		row[m_Columns.m_col_ul] = Formatter::format_size(status.upload_payload_rate) + "/s";
		row[m_Columns.m_col_size] = Formatter::format_size(status.total_wanted);
		if(status.download_payload_rate)
			row[m_Columns.m_col_eta] = Formatter::format_time((status.total_wanted - status.total_wanted_done) / status.download_payload_rate);
	}
}

void TTItemWindow::add_torrent(const Glib::ustring& magnet_url, const Glib::ustring& file_path) {
	
	auto handle = m_TorrentHandler.AddTorrent(magnet_url, file_path);
	add_torrent_row(handle);
	m_Item->torrents.push_back({magnet_url, file_path});
}

void TTItemWindow::add_torrent_row(const lt::torrent_handle& handle) {
	auto status = handle.status();
	auto original = (m_refTreeModel->append());

	Gtk::TreeModel::Row row = *original;
	row[m_Columns.m_col_id] = m_refTreeModel->children().size();
	row[m_Columns.m_col_name] = status.name;
	row[m_Columns.m_col_progress] = status.progress_ppm / 10000;
	row[m_Columns.m_col_state] = TorrentHandler::state(status);
	row[m_Columns.m_col_dl] = Formatter::format_size(status.download_payload_rate) + "/s";
	row[m_Columns.m_col_ul] = Formatter::format_size(status.upload_payload_rate) + "/s";
	row[m_Columns.m_col_size] = Formatter::format_size(status.total_wanted);
	if(status.download_payload_rate)
		row[m_Columns.m_col_eta] = Formatter::format_time((status.total_wanted - status.total_wanted_done) / status.download_payload_rate);
}

void TTItemWindow::remove_selected_rows(bool remove_files) {
	auto selection = m_TreeView.get_selection();
	if(!selection) return;
	auto row = selection->get_selected();
	if(!row) return;
	unsigned int id = row->get_value(m_Columns.m_col_id) - 1;
	auto name = row->get_value(m_Columns.m_col_name);

	std::cout << id << ": " << name << std::endl;
	m_TorrentHandler.RemoveTorrent(name, remove_files);

	if(remove_files) {
		//auto path = m_Item->torrents[id].file_path;
		//ResourceManager::delete_file_with_path(path, name);
		ResourceManager::delete_file(name);
	}

	m_Item->torrents.erase(m_Item->torrents.begin() + id);
	m_refTreeModel->erase(row);
	auto children = m_refTreeModel->children();
	for(int i = 1; i <= children.size(); i++) {
		children[i-1][m_Columns.m_col_id] = i;
	}
	std::cout << "Deleted!" << std::endl;
}

TTItemWindow::~TTItemWindow() {
	delete m_Dialog;
	delete m_RemoveDialog;
	m_TorrentHandler.unsubscribe(subscription);
}

void TTItemWindow::on_button_add() {
	m_Dialog->show();
}

void TTItemWindow::on_button_remove() {
	m_RemoveDialog->show();
}

void TTItemWindow::on_button_settings() {

}

bool TTItemWindow::on_row_pressed(GdkEventButton *ev) {
	if(ev->type == GDK_2BUTTON_PRESS) {
		auto selection = m_TreeView.get_selection();
		auto row = selection->get_selected();
		auto state = m_TorrentHandler.m_Handles[row->get_value(m_Columns.m_col_name)].status().state;
		//if(state != lt::torrent_status::finished && state != lt::torrent_status::seeding) return false;
		unsigned int id = row->get_value(m_Columns.m_col_id) - 1;
		auto path = m_Item->torrents[id].file_path;
		//system(Glib::ustring::format("xdg-open ", "\"", path, "/", row->get_value(m_Columns.m_col_name), "\"").c_str());
		GError *error = nullptr;
		if(!g_app_info_launch_default_for_uri(Glib::ustring::format("file:///", path).c_str()
		, nullptr, &error)) {
			g_warning("Failed to open uri: %s", error->message);
			return false;
		}
		return true;
	}
	return true;
}

void TTItemWindow::on_start_torrent() {
	auto selection = m_TreeView.get_selection();
	if(!selection) return;
	auto row = selection->get_selected();
	if(!row) return;
	auto handle = m_TorrentHandler.m_Handles[row->get_value(m_Columns.m_col_name)];
	handle.auto_managed(true);
	handle.resume();
}

void TTItemWindow::on_pause_torrent() {
	auto selection = m_TreeView.get_selection();
	if(!selection) return;
	auto row = selection->get_selected();
	if(!row) return;
	auto handle = m_TorrentHandler.m_Handles[row->get_value(m_Columns.m_col_name)];
	handle.auto_managed(false);
	handle.pause();
}

void TTItemWindow::on_torrentadddialog_response(int response_id) {
	
	m_Dialog->hide();

	switch(response_id) {
		case Gtk::RESPONSE_OK:
			std::cout << "OK" << std::endl;
			Gtk::Entry* magnet;
			builder->get_widget<Gtk::Entry>("MagnetLink", magnet);
			Gtk::FileChooserButton* file;
			builder->get_widget<Gtk::FileChooserButton>("FilePath", file);

			add_torrent(magnet->get_text(), file->get_filename());

			break;
		case Gtk::RESPONSE_CANCEL:
			std :: cout << "Cancel" << std::endl;
			break;
		default:
			std :: cout << "What" << std::endl;
			break;
	}
}

void TTItemWindow::on_torrentremovedialog_response(int response_id) {
	m_RemoveDialog->hide();

	switch(response_id) {
		case Gtk::RESPONSE_YES:
			std::cout << "Yes" << std::endl;
			Gtk::CheckButton* button;
			remove_builder->get_widget("RemoveFile", button);
			remove_selected_rows(button->get_active());
			break;
		case Gtk::RESPONSE_NO:
			std::cout << "No" << std::endl;
			break;
		default:
			std::cout << "What" << std::endl;
			break;
	}
}
