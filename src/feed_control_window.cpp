//
// Created by simon on 2021. jan. 4..
//

#include "feed_control_window.h"
#include "resource_manager.h"
#include "main_window.h"
#include "macros.h"

TTFeedControlWindow::TTFeedControlWindow(TTMainWindow* caller)
: parent(caller)
{
    auto builder = Gtk::Builder::create_from_file(ResourceManager::get_resource_path("tvtorrent_feedcontrol.glade"));
    builder->get_widget("FeedControlWindow", window);

    builder->get_widget("FilterDownloadName", download_name);

    builder->get_widget("FilterVerPattern", ver_pattern);

    builder->get_widget("TVWidgetChooser", tvw_chooser);

    download_name->ON_CHANGE(&TTFeedControlWindow::on_dl_changed);
    ver_pattern->ON_CHANGE(&TTFeedControlWindow::on_vp_changed);
    tvw_chooser->ON_CHANGE(&TTFeedControlWindow::on_tvw_changed);

    builder->get_widget("FilterList", filter_list);
    filter_list->ON_ROW_SELECTED(&TTFeedControlWindow::on_filter_activate);
    for(auto& filter : caller->GetFilters()) {
            auto filter_list_item = Gtk::make_managed<Gtk::Entry>();
            filter_list_item->set_data("filter_id", reinterpret_cast<void *>(&filter.internal_id));
            filter_list_item->set_text(filter.id);
            m_Filters.push_back(filter_list_item);
            filter_list->append(*filter_list_item);
            filter_list_item->ON_BUTTON_PRESSED_BIND(&TTFeedControlWindow::on_filter_pressed, Gtk::ListBoxRow*), filter_list->get_row_at_index(m_Filters.size() - 1)));
            //filter_list_item->ON_ACTIVATE(&TTFeedControlWindow::on_filter_activate);
    }
    for(auto tvw : caller->tvw_list) {
        tvw_chooser->append(tvw->GetName());
    }
    tvw_chooser->set_active(0);

    builder->get_widget("FeedList", feed_list);

    for(auto feed : caller->feed_list) {
        auto feed_list_item = Gtk::make_managed<Gtk::CheckButton>(feed->channel_data.title);
        feed_list_item->ON_CLICK_BIND(&TTFeedControlWindow::on_feed_list_item_click, Gtk::CheckButton*), feed_list_item));
        m_Feeds.push_back(feed_list_item);
        feed_list->append(*feed_list_item);
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
    window->show_all_children();
    window->show();
}

TTFeedControlWindow::~TTFeedControlWindow() {

    for(auto child : m_Filters) {
        int* id = static_cast<int*>(child->get_data("filter_id"));
        for(auto& filter : parent->GetFilters()) {
            if(filter.internal_id == *id) {
                filter.id = child->get_text();
                break;
            }
        }
    }

    delete window;
}

void TTFeedControlWindow::on_add_filter() {
    auto filter_list_item = Gtk::make_managed<Gtk::Entry>();
    auto& filter = parent->add_filter();
    filter_list_item->set_data("filter_id", reinterpret_cast<void *>(&filter.internal_id));
    m_Filters.push_back(filter_list_item);
    //filter_list_item->ON_ACTIVATE(&TTFeedControlWindow::on_filter_activate);
    filter_list->append(*filter_list_item);
    filter_list->show_all_children();

    filter_list_item->ON_BUTTON_PRESSED_BIND(&TTFeedControlWindow::on_filter_pressed, Gtk::ListBoxRow*), filter_list->get_row_at_index(m_Filters.size() - 1)));
    filter_list->select_row(*filter_list->get_row_at_index(m_Filters.size()-1));

}

void TTFeedControlWindow::on_remove_filter() {
    Gtk::ListBoxRow *selected = filter_list->get_selected_row();
    if(!selected) {
        return;
    }
    auto* selected_as_entry = reinterpret_cast<Gtk::Entry*>(selected->get_child());
    int i = 0;
    int* id = static_cast<int*>(selected_as_entry->get_data("filter_id"));
    for(const auto& filter : parent->GetFilters()) {
        if(filter.internal_id == *id) {
            parent->RemoveFilter(i);
            goto done;
        }
        i++;
    }
done:
    i = 0;
    for(auto entry : m_Filters) {
        if(*id == *static_cast<int*>(entry->get_data("filter_id"))) {
            m_Filters.erase(m_Filters.begin() + i);
            break;
        }
        i++;
    }

    filter_list->remove(*selected);

    selected_filter = nullptr;
    download_name->set_text("");
    ver_pattern->set_text("");
    update_results();
}

void TTFeedControlWindow::on_add_feed() {
    Gtk::Dialog* dialog;
    auto builder = Gtk::Builder::create_from_file(ResourceManager::get_resource_path("tvtorrent_addfeed.glade"));
    builder->get_widget("AddFeedDialog", dialog);

    Gtk::Entry* feedurl;
    builder->get_widget("FeedLink", feedurl);

    int result = dialog->run();

    switch(result) {
        case Gtk::RESPONSE_OK: {
            parent->add_feed(feedurl->get_text());

            auto feed_list_item = Gtk::make_managed<Gtk::CheckButton>(parent->feed_list.back()->channel_data.title);
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

}

void TTFeedControlWindow::on_filter_activate(Gtk::ListBoxRow* row) {
    if(row == nullptr) return;
    int* internal_id = static_cast<int *>(row->get_child()->get_data("filter_id"));
    for(auto& filter : parent->GetFilters()) {
        if(filter.internal_id == *internal_id) {
            if(selected_filter != nullptr && *selected_filter == filter) return;
            selected_filter = &filter;
            break;
        }
    }
    download_name->set_text(selected_filter->name);
    ver_pattern->set_text(selected_filter->ver_pattern);
    tvw_chooser->set_active_text(selected_filter->tvw);
    for(auto feed : m_Feeds) {
        bool found = false;
        auto label = std::move(feed->get_label());
        for(auto& item : selected_filter->feeds) {
            if(item == label) {
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
    if(selected_filter == nullptr) return;
    if(btn->get_active()) {
        auto text = btn->get_label();
        for(auto& feed : selected_filter->feeds) {
            if(feed == text) return;
        }
        selected_filter->feeds.push_back(text);
    }
    else {
        auto text = btn->get_label();
        int i = 0;
        for(auto& feed : selected_filter->feeds) {
            if(feed == text) {
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
        for(const auto& item : parent->feed_list) {
            if(item->channel_data.title == feed) {
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

bool TTFeedControlWindow::on_filter_pressed(GdkEventButton* ev, Gtk::ListBoxRow* row) {
    filter_list->select_row(*row);
    return true;
}
