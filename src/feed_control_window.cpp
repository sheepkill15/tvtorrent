//
// Created by simon on 2021. jan. 4..
//

#include "feed_control_window.h"
#include "resource_manager.h"
#include "main_window.h"
#include "macros.h"
#include "logger.h"
#include "container.h"
TTFeedControlWindow::TTFeedControlWindow()
    : m_Dispatcher()
{
    m_Running++;
    if(m_Running > 1) {
        delete this;
        return;
    }
    Logger::watcher w("Initializing Feed control window!");
    auto builder = Gtk::Builder::create_from_file(ResourceManager::get_resource_path("tvtorrent_feedcontrol.glade"));
    builder->get_widget("FeedControlWindow", window);

    builder->get_widget("FilterDownloadName", download_name);

    builder->get_widget("FilterVerPattern", ver_pattern);

    builder->get_widget("TVWidgetChooser", tvw_chooser);

    download_name->ON_CHANGE(&TTFeedControlWindow::on_dl_changed);
    ver_pattern->ON_CHANGE(&TTFeedControlWindow::on_vp_changed);
    tvw_chooser->ON_CHANGE(&TTFeedControlWindow::on_tvw_changed);

    m_Dispatcher.ON_DISPATCH(&TTFeedControlWindow::notify_feed_changed);

    builder->get_widget("FilterList", filter_list);
    filter_list->ON_ROW_SELECTED(&TTFeedControlWindow::on_filter_activate);
    for(auto filter : DataContainer::get_filters()) {
            auto filter_list_item = Gtk::make_managed<Gtk::Entry>();
            filter_list_item->set_text(filter->id);
            filter_list->append(*filter_list_item);
            filter_list_item->ON_BUTTON_PRESSED_BIND(&TTFeedControlWindow::on_filter_pressed, Gtk::ListBoxRow*), filter_list->get_row_at_index(filter_list->get_children().size() -1)));
            //filter_list_item->ON_ACTIVATE(&TTFeedControlWindow::on_filter_activate);
    }
    for(auto& group : DataContainer::get_groups()) {
        tvw_chooser->append(group.first->name);
    }
    tvw_chooser->set_active(0);

    builder->get_widget("FeedList", feed_list);

    for(auto feed : DataContainer::get_feeds()) {
        auto feed_list_item = Gtk::make_managed<Gtk::CheckButton>(feed->channel_data.title);
        feed_list_item->set_data("feed", reinterpret_cast<void *>(feed));
        feed_list_item->ON_CLICK_BIND(&TTFeedControlWindow::on_feed_list_item_click, Gtk::CheckButton*), feed_list_item));
        m_Feeds.push_back(feed_list_item);
        feed_list->append(*feed_list_item);
        m_Subscriptions.insert(std::make_pair(feed->channel_data.hash, feed->subscribe([this] { m_Dispatcher.emit(); })));
    }

    builder->get_widget("ResultList", result_list);

    Gtk::Button* add_filter;
    builder->get_widget("AddFilter", add_filter);

    Gtk::Button* remove_filter;
    builder->get_widget("RemoveFilter", remove_filter);

    Gtk::Button* add_feed;
    builder->get_widget("AddFeed", add_feed);

    Gtk::Button* remove_feed;
    builder->get_widget("RemoveFeed", remove_feed);

    add_filter->ON_CLICK(&TTFeedControlWindow::on_add_filter);
    remove_filter->ON_CLICK(&TTFeedControlWindow::on_remove_filter);
    add_feed->ON_CLICK(&TTFeedControlWindow::on_add_feed);
    remove_feed->ON_CLICK(&TTFeedControlWindow::on_remove_feed);

    window->ON_HIDE(&TTFeedControlWindow::self_destruct);
    window->show_all_children();
    window->show();

    DataContainer::get_manager().pause();
    m_Running = true;
}

TTFeedControlWindow::~TTFeedControlWindow() {
    m_Running--;
    if(filter_list != nullptr) {
        for (int i = 0; i < filter_list->get_children().size(); i++) {

            Feed::Filter *filter = DataContainer::get_filters()[i];
            filter->id = reinterpret_cast<Gtk::Entry *>(filter_list->get_row_at_index(i)->get_child())->get_text();
        }
    }

    for(auto& pair : m_Subscriptions) {
        DataContainer::get_feed(pair.first)->unsubscribe(pair.second);
    }

    delete window;
}

void TTFeedControlWindow::on_add_filter() {
    Logger::watcher w("Adding filter");

    auto filter_list_item = Gtk::make_managed<Gtk::Entry>();
    DataContainer::add_filter();
    filter_list->append(*filter_list_item);
    filter_list->show_all_children();

    filter_list_item->ON_BUTTON_PRESSED_BIND(&TTFeedControlWindow::on_filter_pressed, Gtk::ListBoxRow*), filter_list->get_row_at_index(filter_list->get_children().size() -1)));
    filter_list->select_row(*filter_list->get_row_at_index(filter_list->get_children().size()-1));
}

void TTFeedControlWindow::on_remove_filter() {
    Logger::watcher w("Removing filter");

    Gtk::ListBoxRow *selected = filter_list->get_selected_row();
    if(!selected) {
        return;
    }
    int i = 0;
    Feed::Filter* filt = DataContainer::get_filters()[selected->get_index()];
    DataContainer::remove_filter(filt->internal_id);
    filter_list->remove(*selected);

    selected_filter = nullptr;
    download_name->set_text("");
    ver_pattern->set_text("");
    update_results();
}

void TTFeedControlWindow::on_add_feed() {
    Logger::watcher w("Adding feed");

    Gtk::Dialog* dialog;
    auto builder = Gtk::Builder::create_from_file(ResourceManager::get_resource_path("tvtorrent_addfeed.glade"));
    builder->get_widget("AddFeedDialog", dialog);

    Gtk::Entry* feedurl;
    builder->get_widget("FeedLink", feedurl);

    int result = dialog->run();

    switch(result) {
        case Gtk::RESPONSE_OK: {
            DataContainer::add_feed(feedurl->get_text());

            auto feed_list_item = Gtk::make_managed<Gtk::CheckButton>(DataContainer::get_feeds().back()->channel_data.title);
            auto feed = DataContainer::get_feeds().back();
            feed_list_item->set_data("feed", reinterpret_cast<void *>(feed));
            m_Subscriptions.insert(std::make_pair(feed->channel_data.hash, feed->subscribe([this] { TTFeedControlWindow::notify_feed_changed(); })));
            m_Feeds.push_back(feed_list_item);
            feed_list_item->ON_CLICK_BIND(&TTFeedControlWindow::on_feed_list_item_click, Gtk::CheckButton*), feed_list_item));

            feed_list->append(*feed_list_item);
            feed_list->show_all_children();
        }
            break;
        case Gtk::RESPONSE_CANCEL:
        default:
            break;
    }

    dialog->hide();
    delete dialog;
}

void TTFeedControlWindow::on_remove_feed() {
    auto builder = Gtk::Builder::create_from_file(ResourceManager::get_resource_path("tvtorrent_removefeed.glade"));
    Gtk::Dialog* dialog;
    builder->get_widget("RemoveFeedDialog", dialog);

    Gtk::ComboBox* cbt;
    builder->get_widget("FeedList", cbt);

    auto treeModel = Gtk::ListStore::create(m_Column);

    cbt->set_model(treeModel);

    cbt->pack_start(m_Column.m_col_name);

    for(auto& feed : DataContainer::get_feeds()) {
        Gtk::TreeModel::Row row = *treeModel->append();
        row[m_Column.m_col_name] = feed->channel_data.title;
        row[m_Column.m_col_feed] = feed->channel_data.hash;
    }

    cbt->set_active(0);

    int result = dialog->run();

    switch(result) {
        case Gtk::RESPONSE_OK: {
            auto row = cbt->get_active();
            DataContainer::remove_feed(row->get_value(m_Column.m_col_feed));
            UpdateFeeds(cbt->get_active_row_number());
            m_Subscriptions.erase(row->get_value(m_Column.m_col_feed));
            break;
        }
        case Gtk::RESPONSE_CANCEL:
        default:
            break;
    }

    dialog->hide();
    delete dialog;

}

void TTFeedControlWindow::on_filter_activate(Gtk::ListBoxRow* row) {
    Logger::watcher w("Filter activated");
    row = filter_list->get_selected_row();
    if(row == nullptr) return;
    int inddd = row->get_index();
    selected_filter = DataContainer::get_filters()[inddd];
    download_name->set_text(selected_filter->name);
    ver_pattern->set_text(selected_filter->ver_pattern);
    tvw_chooser->set_active_text(selected_filter->tvw);
    for(auto feed : m_Feeds) {
        bool found = false;
        size_t id = static_cast<Feed*>(feed->get_data("feed"))->channel_data.hash;
        for(auto& item : selected_filter->feeds) {
            if(item == id) {
                found = true;
                break;
            }
        }
        feed->set_active(found);
    }
    update_results();
}

void TTFeedControlWindow::on_dl_changed() {
    if(selected_filter == nullptr) return;

    selected_filter->name = std::move(download_name->get_text());
    update_results();
}

void TTFeedControlWindow::on_vp_changed() {
    if(selected_filter == nullptr) return;

    selected_filter->ver_pattern = std::move(ver_pattern->get_text());

}

void TTFeedControlWindow::on_tvw_changed() {
    if(selected_filter == nullptr) return;

    selected_filter->tvw = std::move(tvw_chooser->get_active_text());
}

void TTFeedControlWindow::on_feed_list_item_click(Gtk::CheckButton* btn) {
    Logger::watcher w("FeedListItem clicked");
    if(selected_filter == nullptr) return;
    if(btn->get_active()) {
        size_t id = static_cast<Feed*>(btn->get_data("feed"))->channel_data.hash;
        for(auto& feed : selected_filter->feeds) {
            if(feed == id) return;
        }
        selected_filter->feeds.push_back(id);
    }
    else {
        size_t id = static_cast<Feed*>(btn->get_data("feed"))->channel_data.hash;
        int i = 0;
        for(auto& feed : selected_filter->feeds) {
            if(feed == id) {
                selected_filter->feeds.erase(selected_filter->feeds.begin() + i);
                break;
            }
            i++;
        }
    }

    update_results();
}

namespace {
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

void TTFeedControlWindow::update_results() {
    if(selected_filter == nullptr) return;
    for(auto child : result_list->get_children()) {
        result_list->remove(*child);
    }
    std::vector<std::string> name_split;
    split(selected_filter->name, name_split, ' ');
    for(const auto& feed : selected_filter->feeds) {
        for(const auto& item : DataContainer::get_feeds()) {
            if(item->channel_data.hash == feed) {
                for(const auto& feed_item : item->GetItems()) {
                    bool ok = true;
                    for(auto& s : name_split) {
                        if(feed_item.title.find(s) == std::string::npos)
                            ok = false;
                    }
                    if(ok) {
                        result_list->append(*Gtk::make_managed<Gtk::Label>(feed_item.title));
                    }
                }
                break;
            }
        }
    }
    result_list->show_all_children();
}

bool TTFeedControlWindow::on_filter_pressed(GdkEventButton* _, Gtk::ListBoxRow* row) {
    filter_list->select_row(*row);
    return true;
}

void TTFeedControlWindow::UpdateFeeds(size_t index) {
    feed_list->remove(*feed_list->get_row_at_index(index));
}

void TTFeedControlWindow::notify_feed_changed() {
    for(auto entry : m_Feeds) {
        Feed* feed = static_cast<Feed*>(entry->get_data("feed"));
        if(feed == nullptr) continue;
        entry->set_label(feed->channel_data.title);
    }
    update_results();
}

void TTFeedControlWindow::self_destruct() {
    DataContainer::get_manager().resume();
    DataContainer::get_manager().refresh_check();
    delete this;
}
