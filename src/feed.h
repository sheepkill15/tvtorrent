#ifndef TVTORRENT_FEED_H
#define TVTORRENT_FEED_H

#include <mutex>
#include <rapidxml/rapidxml.hpp>
#include <curl/curl.h>
#include <curl/easy.h>
#include <string>
#include <thread>
#include <vector>

class Feed {
public:
    struct Item {
        std::string title;
        std::string link;
        std::string date;
    };

    struct Filter {
        std::string tvwidget;
        std::string name;
        std::string ver_pattern;
    };

    explicit Feed(std::string );
    ~Feed();
    void parse_feed(bool = false);
    void add_filter(const std::string&, const std::string&, const std::string&);

    struct {
        std::string title;
        std::string desc;
        std::string link;
    } channel_data;

private:
    void periodic();
    void parse_item(rapidxml::xml_node<char>*);

    std::string RSS_URL;
    std::string buffer;

    rapidxml::xml_document<> doc;

    void* curl{};

    mutable std::mutex m_Mutex;
    std::thread own;

    bool should_work{};

    std::vector<Item> m_Items;
    std::vector<Filter> m_Filters;
};

#endif