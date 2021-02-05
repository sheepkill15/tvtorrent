#include "feed.h"
#include "resource_manager.h"
#include <utility>
#include "logger.h"
#include "downloader.h"

Feed::Feed(std::string  rss_url)
    : channel_data {.hash = Unique::from_string(rss_url)},
    RSS_URL(std::move(rss_url))
{
    //parse_feed(true);
    own = std::thread([this] {periodic();});
}

Feed::~Feed() {
    //curl_easy_cleanup(curl);
    should_work = false;
    own.detach();
//    if(own.joinable()) {
//        own.join();
//    }
}

//void Feed::add_filter(const std::string& tvwidget, const std::string & name, const std::string & pattern) {
//    m_Filters.push_back({tvwidget, name, pattern});
//}

void Feed::periodic() {
    should_work = true;
    while(should_work) {
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            Logger::watcher w("Parsing feed" + channel_data.title);
            parse_feed();
        }
	    std::this_thread::sleep_for(std::chrono::minutes(5));
    }
}

void Feed::parse_feed() {
    {
        buffer.clear();
        Downloader::fetch_no_alloc(RSS_URL, buffer);
        doc.clear();
        try {
            doc.parse<0>(buffer.data());
        } catch (rapidxml::parse_error &er) {
            Logger::error(er.what());
            return;
        }
        auto root = doc.first_node();
        if (std::string(root->name()) != "rss") return;
        auto channel = root->first_node("channel");

        auto title = channel->first_node("title");
        auto desc = channel->first_node("description");
        channel_data.title = title->value();
        auto pos = RSS_URL.find("rss");
        if (pos != std::string::npos && pos < RSS_URL.size() - 4) {
            channel_data.title += " (" + RSS_URL.substr(pos + 4) + ")";
        }

        channel_data.desc = desc->value();
        m_Items.clear();
        for (auto child = channel->first_node("item"); child != nullptr; child = child->next_sibling("item")) {
            parse_item(child);
        }
    }

    for(auto& sb : m_Subscriptions) {
        sb.second();
    }

//    for(auto& item : m_Items) {
//        std::cout << item.title << std::endl << item.link << std::endl << item.date << std::endl;
//    }

	//std::cout << "Name of first node is: " << doc.first_node()->name() << std::endl;
}

void Feed::parse_item (rapidxml::xml_node<>* child) {
    auto title = child->first_node("title");
    if(!title) return;
    std::string title_value = title->value();
    if(title_value.empty()) {
        auto first_title = title->first_node();
        if(first_title) {
            title_value = first_title->value();
        }
    }

    auto link = child->first_node("link");
    if(!link) return;
    std::string link_value = link->value();

    auto pubDate = child->first_node("pubDate");
    std::string pubDate_value;
    if(pubDate)
        pubDate_value = pubDate->value();


    m_Items.push_back({title_value, link_value, pubDate_value});
}

bool Feed::operator==(size_t h) const {
    return channel_data.hash == h;
}

size_t Feed::subscribe(const std::function<void()>& sbc) {
    m_Subscriptions.insert(std::make_pair(subscriptions++, sbc));
    return subscriptions - 1;
}

void Feed::unsubscribe(size_t subscription) {
    m_Subscriptions.erase(subscription);
}
