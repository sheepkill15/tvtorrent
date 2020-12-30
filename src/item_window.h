#ifndef TVTORRENT_ITEM_WINDOW_H
#define TVTORRENT_ITEM_WINDOW_H

#include "glibmm/refptr.h"
#include "gtkmm/builder.h"
#include "gtkmm/liststore.h"
#include "gtkmm/scrolledwindow.h"
#include "gtkmm/treemodel.h"
#include "gtkmm/treemodelcolumn.h"
#include "gtkmm/treeview.h"
#include "gtkmm/window.h"
#include "torrent_handler.h"
#include "tv_item.h"
#include "tv_widget.h"
#include <gtkmm/messagedialog.h>
#include <unordered_map>
#include <glibmm/dispatcher.h>

class ModelColumns : public Gtk::TreeModel::ColumnRecord {
	public:
		ModelColumns() { add(m_col_id); add(m_col_name); add(m_col_progress); add(m_col_state); add(m_col_dl); add(m_col_ul); add(m_col_size); add(m_col_eta); }

		Gtk::TreeModelColumn<unsigned int> m_col_id;
		Gtk::TreeModelColumn<Glib::ustring> m_col_name;
		Gtk::TreeModelColumn<int> m_col_progress;
		Gtk::TreeModelColumn<Glib::ustring> m_col_state;
		Gtk::TreeModelColumn<Glib::ustring> m_col_dl;
		Gtk::TreeModelColumn<Glib::ustring> m_col_ul;
		Gtk::TreeModelColumn<Glib::ustring> m_col_size;
		Gtk::TreeModelColumn<Glib::ustring> m_col_eta;
};

class TTItemWindow : public Gtk::Window {

public:

	TTItemWindow(TVWidget& item);
	~TTItemWindow() override;

	void on_button_add();
	void on_button_remove();
	void on_button_settings();
	void on_torrentadddialog_response(int response_id);
	void on_torrentremovedialog_response(int response_id);
	bool on_row_pressed(GdkEventButton* ev);
	void on_start_torrent();
	void on_pause_torrent();

	void add_torrent(const Glib::ustring& magnet_url, const Glib::ustring& file_path);
	//void update_torrents(TTItemWindow* caller);
	void update_torrent_views();
	void add_torrent_row(const lt::torrent_handle& handle);
	void remove_selected_rows(bool remove_files);

	void notify();

protected:
	Gtk::Box m_Box;
	Gtk::ScrolledWindow m_ScrolledWindow;
	Gtk::TreeView m_TreeView;
	Glib::RefPtr<Gtk::ListStore> m_refTreeModel;

	Gtk::MessageDialog* m_Dialog;
	Gtk::MessageDialog* m_RemoveDialog;

private:
	TorrentHandler& m_TorrentHandler;
	ModelColumns m_Columns;
	TVItem* m_Item;

	Glib::RefPtr<Gtk::Builder> builder;
	Glib::RefPtr<Gtk::Builder> remove_builder;

	Glib::Dispatcher m_Dispatcher;

	int subscription;
};

#endif
