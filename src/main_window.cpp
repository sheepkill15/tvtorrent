#include "main_window.h"
#include "gtkmm/filechooser.h"
#include "gtkmm/filechooserbutton.h"
#include <gtkmm/toolbar.h>
#include <gtkmm/comboboxtext.h>
#include <memory>
#include "macros.h"
#include "resource_manager.h"
#include "feed_control_window.h"
#include "settings_manager.h"
#include "logger.h"

TTMainWindow::TTMainWindow()
	: tvw_list()
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


    check = std::thread([this] {check_feeds();});
}

TTMainWindow::~TTMainWindow() {

    ResourceManager::create_torrent_save(tvw_list);

    std::vector<Glib::ustring> feeds;
    for(auto& feed : feed_list) {
        feeds.emplace_back(feed->GetUrl());
    }

    ResourceManager::create_feed_save(feeds, m_Filters, m_Downloaded);

    SettingsManager::save();

	for(auto tvw : tvw_list) {
		delete tvw;
	}

	for(auto feed : feed_list) {
	    delete feed;
	}

	should_work = false;
	check.detach();

	Logger::cleanup();
}

void TTMainWindow::external_torrent(char argv[]) {
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
			auto tvw = tvw_list[item_list->get_active_row_number()];
			tvw->GetItem().torrents.push_back({argv, file_path->get_filename()});
			tvw->GetHandler().AddTorrent(argv, file_path->get_filename());
			break;
		}
	    case Gtk::RESPONSE_NO:
		default:
			break;
	}
	dialog->hide();
	delete dialog;
}

void TTMainWindow::init_items() {
	Json::Value root;

	bool ok = ResourceManager::get_torrent_save(root);

	if(ok) {
		for(int i = 0; i < root.size(); i++) {
		    if(!root[i].isMember("default_path")) continue;
			add_item(root[i]["name"].asString(), root[i]["img_path"].asString(), root[i]["default_path"].asString());
			if(root[i].isMember("torrents") && root[i]["torrents"]) {
				for(int j = 0; j < root[i]["torrents"].size(); j++) {
				    std::string uri = root[i]["torrents"][j]["magnet_uri"].asString();
				    for(auto& torrent : tvw_list[i]->GetItem().torrents) {
				        if(torrent.magnet_uri == uri) {
				            goto skip;
				        }
				    }
					tvw_list[i]->GetItem().torrents.push_back({uri, root[i]["torrents"][j]["file_path"].asString()});
skip:
				    ;
				}
				tvw_list.back()->SetupTorrents();
			}
		}
	}
	Json::Value feed_root;

	ok = ResourceManager::get_feed_save(feed_root);
	if(ok) {
	    for(const auto & i : feed_root["feeds"]) {
	        add_feed(i.asString());
	    }
	    for(const auto& i : feed_root["filters"]) {
            auto& filter =  m_Filters.emplace_back();
            filter.internal_id = filter_count++;
            filter.id = i["id"].asString();
            filter.name = i["name"].asString();
            filter.ver_pattern = i["ver_pattern"].asString();
            filter.tvw = i["tvw"].asString();
            if(i.isMember("feeds") && i["feeds"]) {
                for(const auto& j : i["feeds"]) {
                    filter.feeds.emplace_back(j.asString());
                }
            }
	    }

	    for(const auto& i : feed_root["downloads"]) {
	        m_Downloaded.push_back(i.asString());
	    }
	}
}

void TTMainWindow::add_item(const Glib::ustring& name, const Glib::ustring& img_path, const Glib::ustring& default_path) {

	if(name.empty() || img_path.empty() || default_path.empty()) return;

	tvw_list.push_back(new TVWidget(name, img_path, default_path));
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
	delete tvw;
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

void TTMainWindow::on_button_feeds() {
    if(feed_control_window != nullptr) {
        feed_control_window->window->show();
        return;
    }
    feed_control_window = new TTFeedControlWindow(this);
    feed_control_window->window->ON_HIDE(&TTMainWindow::on_feedcontrol_window_hide);
}

void TTMainWindow::on_feedcontrol_window_hide() {
    delete feed_control_window;
    feed_control_window = nullptr;
    refresh_check();
}

void TTMainWindow::add_feed(const Glib::ustring &url) {
    feed_list.push_back(new Feed(url));
    auto feed = feed_list.back();
    for(int i = 0; i < feed_list.size() - 1; i++) {
        if(feed_list[i]->channel_data.title == feed->channel_data.title) {
            feed->channel_data.title += " (1)";
        }
    }
}

Feed::Filter& TTMainWindow::add_filter() {
    auto& filter =  m_Filters.emplace_back();
    filter.internal_id = filter_count++;
    return filter;
}

namespace {
    void ReplaceAll(std::string &str, const std::string& from, const std::string& to) {
        size_t start_pos = 0;
        while((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
        }
    }
    size_t split(const std::string &txt, std::vector<std::string> &strs, char ch)
    {
        size_t pos = txt.find( ch );
        size_t initialPos = 0;
        strs.clear();

        // Decompose statement
        while( pos != std::string::npos ) {
            strs.push_back( txt.substr( initialPos, pos - initialPos ) );
            initialPos = pos + 1;

            pos = txt.find( ch, initialPos );
        }

        // Add the last one
        strs.push_back( txt.substr( initialPos, std::min( pos, txt.size() ) - initialPos + 1 ) );

        return strs.size();
    }
}



void TTMainWindow::check_feeds() {
    should_work = true;
    while(should_work) {
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
//            for(auto& feed : feed_list) {
//                feed->parse_feed();
//            }
            if(feed_control_window != nullptr) goto skip;
            for(auto& filter : m_Filters) {
                if(filter.tvw.empty() || filter.name.empty()) continue;
                std::string filter_processed = "\\b" + filter.ver_pattern;
                ReplaceAll(filter_processed, "X", "[0-9]");
                filter_processed += "\\b";
                std::regex re(filter_processed);
                std::vector<std::string> name_split;
                split(filter.name, name_split, ' ');
                for(auto& feed : feed_list) {
                    for(auto& item : feed->GetItems()) {
                        bool ok = true;
                        for(auto& s : name_split) {
                            if(item.title.find(s) == std::string::npos)
                                ok = false;
                        }
                        if(ok) {
                            std::smatch m;
                            bool const matched = std::regex_search(item.title, m, re);
                            if(matched) {
                                bool const ifAlreadyDownloaded = check_if_already_downloaded(name_split, m);
                                if(ifAlreadyDownloaded) {
                                    m_Downloaded.push_back(item.title);
                                    for(auto tvw : tvw_list) {
                                        if(tvw->GetName() == filter.tvw) {
                                            for(auto& torrent : tvw->GetItem().torrents) {
                                                if(torrent.magnet_uri == item.link)
                                                    continue;
                                            }
                                            tvw->GetHandler().AddTorrent(item.link, tvw->GetItem().default_save_path);
                                            tvw->GetItem().torrents.push_back({item.link, tvw->GetItem().default_save_path});
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

        }
skip:

        std::this_thread::sleep_for(std::chrono::minutes(5));
    }
}

bool TTMainWindow::check_if_already_downloaded(const std::vector<std::string>& name, const std::smatch& m) {
    for(auto& item : m_Downloaded) {
        bool ok = true;
        for(auto& s : name) {
            if(item.find(s) == std::string::npos) {
                ok = false;
            }
        }
        if(ok && (m[0].str().empty() || item.find(m[0]))) {
            return false;
        }
    }
    return true;
}

void TTMainWindow::on_settings_window_hide() {
    delete settings_window;
    settings_window = nullptr;
}

void TTMainWindow::update_limits() {
    for(auto tvw : tvw_list) {
        tvw->GetHandler().update_limits();
    }
}

void TTMainWindow::refresh_check() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    should_work = false;
    check.detach();

    check = std::thread([this] {check_feeds();});
}

