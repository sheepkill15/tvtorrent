#ifndef TVTORRENT_MAIN_WINDOW_H
#define TVTORRENT_MAIN_WINDOW_H

#include "feed.h"
#include "gdkmm/event.h"
#include "glibmm/refptr.h"
#include "gtkmm/builder.h"
#include "gtkmm/button.h"
#include "gtkmm/buttonbox.h"
#include "gtkmm/flowbox.h"
#include "gtkmm/headerbar.h"
#include "gtkmm/hvbox.h"
#include "item_window.h"
#include "tv_widget.h"
#include "settings_window.h"
#include <gtkmm/window.h>
#include <gtkmm/scrolledwindow.h>
#include <regex>

class TTFeedControlWindow;

class TTMainWindow : public Gtk::Window {

public:
	explicit TTMainWindow(std::function<void(const std::string&)>);
	~TTMainWindow() override;

	void external_torrent(char argv[]);
    void add_feed(const Glib::ustring& url);
    void update_limits();
    Feed::Filter& add_filter();


    inline std::vector<Feed::Filter>& GetFilters() { return m_Filters; }
    inline void RemoveFilter(int index) { m_Filters.erase(m_Filters.begin() + index); }

    std::vector<TVWidget*> tvw_list;
    std::vector<Feed*> feed_list;
    inline static std::vector<std::string> m_Downloaded;
protected:
    //void on_button_download();
	void on_button_add();
	void on_button_remove();
	void on_button_settings();
	void on_button_feeds();
	bool on_tvwidget_double_click(GdkEventButton* ev);
	void on_item_window_hide(TTItemWindow* window);
	void on_feedcontrol_window_hide();
	void on_settings_window_hide();

	void on_torrent_complete(const lt::torrent_status&);

	void add_item(const Glib::ustring& name, const Glib::ustring& img_path, const Glib::ustring& default_path);
	void init_items();

	void refresh_check();
	void check_feeds();
	static bool check_if_already_downloaded(const std::vector<std::string>&, const std::smatch&);

	std::thread check;
    mutable std::mutex m_Mutex;
    bool should_work{};

	Gtk::FlowBox m_FlowBox;
	Gtk::ScrolledWindow m_ScrolledWindow;
	Gtk::VBox m_VBox;

    Glib::RefPtr<Gtk::Builder> m_refBuilder;

    TTFeedControlWindow* feed_control_window = nullptr;
    TTSettingsWindow* settings_window = nullptr;
    std::vector<Feed::Filter> m_Filters;

    std::function<void(const std::string&)> notify_cb;

    int filter_count = 0;
};

#endif
