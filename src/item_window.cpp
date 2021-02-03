#include "giomm/simpleactiongroup.h"
#include "gtkmm/box.h"
#include "gtkmm/button.h"
#include "tv_widget.h"
#include "item_window.h"
#include <filesystem>
#include <set>
#include <gtkmm/cellrendererprogress.h>
#include <gtkmm/filechooserbutton.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/toolbar.h>
#include "macros.h"
#include "formatter.h"
#include "resource_manager.h"
#include "logger.h"
#include "container.h"

#if defined(WIN32) || defined(WIN64)
#include <shellapi.h>
#include <shlobj.h>

#endif

TTItemWindow::TTItemWindow(size_t hash)
	: hash(hash),
	m_Dispatcher(),
	m_Dispatcher_for_added(),
	m_Box(Gtk::ORIENTATION_VERTICAL)
{
    auto group = DataContainer::get_group(hash);
	set_title(group.first->name);
	set_border_width(10);
	set_default_size(1280, 720);
	
	auto m_refBuilder = Gtk::Builder::create_from_file(ResourceManager::get_resource_path("tvtorrent_headerbar.glade"));

	Gtk::HeaderBar* headerbar = nullptr;
	m_refBuilder->get_widget("HeaderBar", headerbar);
	headerbar->set_title(group.first->name);

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
	path->set_filename(group.first->default_save_path);

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

	Logger::info("Set up item window children");

	show_all_children();

	for(int i = 0; i < group.second->m_Handles.size(); i++) {
		add_torrent_row();
	}

	Logger::info("Added rows");

	m_TreeView.ON_BUTTON_PRESSED(&TTItemWindow::on_row_pressed);

	m_Dispatcher.ON_DISPATCH(&TTItemWindow::update_torrent_views);
	m_Dispatcher_for_added.ON_DISPATCH(&TTItemWindow::add_torrent_row);

	subscription_for_added = group.second->subscribe_for_added((const std::function<void()>&) [this] {TTItemWindow::notify_added(); });
	subscription = group.second->subscribe((const std::function<void()> &) [this] { TTItemWindow::notify(); });

	Logger::info("Subscriptions and dispatchers initialized");

	ON_HIDE(&TTItemWindow::self_destruct);
}

void TTItemWindow::notify() {
	m_Dispatcher.emit();
}

void TTItemWindow::update_torrent_views() {
	for(auto& row: m_refTreeModel->children()) {
	    libtorrent::torrent_handle handle = row->get_value(m_Columns.m_col_hndl);
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

void TTItemWindow::add_torrent(const Glib::ustring& magnet_url, const Glib::ustring& file_path) const {
    Logger::watcher w("Adding torrent" + magnet_url);

    DataContainer::add_torrent(hash, magnet_url, file_path);
}

void TTItemWindow::add_torrent_row() {
    Logger::watcher w("Adding row");
    std::set<std::string> names;
    for(auto& row : m_refTreeModel->children()) {
        names.insert(row->get_value(m_Columns.m_col_name));
    }

    auto group = DataContainer::get_group(hash);
    lt::torrent_status status;
    bool found;
    for(auto& hl : group.second->m_Handles) {
        if(!hl.second.is_valid()) continue;
        auto stat = hl.second.status();
        if(names.find(stat.name) == names.end()) {
            status = stat;
            found = true;
            break;
        }
    }
    if(!found) return;
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
	row[m_Columns.m_col_hndl] = status.handle;
}

void TTItemWindow::remove_selected_rows(bool remove_files) {
    Logger::watcher w("Removing row");
	auto selection = m_TreeView.get_selection();
	if(!selection) return;
	auto row = selection->get_selected();
	if(!row) return;
	unsigned int id = row->get_value(m_Columns.m_col_id) - 1;
	auto name = row->get_value(m_Columns.m_col_name);
    DataContainer::remove_torrent(hash, row->get_value(m_Columns.m_col_hndl), name, id, remove_files);
	m_refTreeModel->erase(row);
	auto children = m_refTreeModel->children();
	for(int i = 1; i <= children.size(); i++) {
		children[i-1][m_Columns.m_col_id] = i;
	}
}

TTItemWindow::~TTItemWindow() {
	delete m_Dialog;
	delete m_RemoveDialog;
	auto group = DataContainer::get_group(hash);
	group.second->unsubscribe(subscription);
	group.second->unsubscribe_from_added(subscription_for_added);
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
	    Logger::watcher w("Double clicked row");
		auto selection = m_TreeView.get_selection();
		if(!selection) return false;
		auto row = selection->get_selected();
		if(!row) return false;
		auto path = row->get_value(m_Columns.m_col_hndl).status().save_path;
#if defined(WIN32) || defined(WIN64)
		auto pidl = ILCreateFromPath(ResourceManager::create_path(path, row->get_value(m_Columns.m_col_name)).c_str());
		if(pidl) {
		    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		    SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0);
		    ILFree(pidl);
		}
    //ShellExecuteA(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
		GError *error = nullptr;
		if(!g_app_info_launch_default_for_uri(Glib::ustring::format("file:///", path).c_str()
		, nullptr, &error)) {
			g_warning("Failed to open uri: %s", error->message);
			return false;
		}
#endif
		return true;
	}
	return true;
}

void TTItemWindow::on_start_torrent() {
    Logger::watcher w("Starting torrent");
	auto selection = m_TreeView.get_selection();
	if(!selection) return;
	auto row = selection->get_selected();
	if(!row) return;
	auto handle = row->get_value(m_Columns.m_col_hndl);
    handle.set_flags(lt::torrent_flags::auto_managed);
	handle.resume();
}

void TTItemWindow::on_pause_torrent() {
    Logger::watcher w("Pausing torrent");
	auto selection = m_TreeView.get_selection();
	if(!selection) return;
	auto row = selection->get_selected();
	if(!row) return;
    auto handle = row->get_value(m_Columns.m_col_hndl);
	handle.unset_flags(lt::torrent_flags::auto_managed);
	handle.pause();
}

void TTItemWindow::on_torrentadddialog_response(int response_id) {

	m_Dialog->hide();

	switch(response_id) {
		case Gtk::RESPONSE_OK:
			Gtk::Entry* magnet;
			builder->get_widget<Gtk::Entry>("MagnetLink", magnet);
			Gtk::FileChooserButton* file;
			builder->get_widget<Gtk::FileChooserButton>("FilePath", file);

			add_torrent(magnet->get_text(), file->get_filename());

			break;
		case Gtk::RESPONSE_CANCEL:
		default:
			break;
	}
}

void TTItemWindow::on_torrentremovedialog_response(int response_id) {
	m_RemoveDialog->hide();

	switch(response_id) {
		case Gtk::RESPONSE_YES:
			Gtk::CheckButton* button;
			remove_builder->get_widget("RemoveFile", button);
			remove_selected_rows(button->get_active());
			break;
		case Gtk::RESPONSE_NO:
		default:
			break;
	}
}

void TTItemWindow::notify_added() {
    m_Dispatcher_for_added.emit();
}

void TTItemWindow::self_destruct() {
    delete this;
}
