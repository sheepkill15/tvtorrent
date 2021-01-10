#ifndef TVTORRENT_FEED_H
#define TVTORRENT_FEED_H

#include <mutex>
#include <rapidxml/rapidxml.hpp>
#include <curl/curl.h>
#include <curl/easy.h>
#include <string>
#include <thread>
#include <vector>
#include <glibmm/ustring.h>
class Feed {
public:
    struct Item {
        std::string title;
        std::string link;
        std::string date;
    };

    struct Filter {
        Glib::ustring id;
        Glib::ustring name;
        Glib::ustring ver_pattern;
        Glib::ustring tvw;
        std::vector<size_t> feeds = {};

        size_t internal_id;

        bool operator== (const Filter& other) const {
            return internal_id == other.internal_id;
        }
        bool operator== (size_t i) const {
            return internal_id == i;
        }
    };

    explicit Feed(std::string );
    ~Feed();
    void parse_feed();

    static size_t writer(char *data, size_t size, size_t nmemb, std::string *buffer);

    //void add_filter(const std::string&, const std::string&, const std::string&);

    inline const std::string& GetUrl() const { return RSS_URL; }
    inline const std::vector<Item>& GetItems() const { return m_Items; }

    struct {
        std::string title;
        std::string desc;
        const size_t hash;
    } channel_data;

    bool operator==(size_t h) const;

private:
    void periodic();
    void parse_item(rapidxml::xml_node<char>*);

    const std::string RSS_URL;

    std::string buffer;

    rapidxml::xml_document<> doc;

    void* curl{};

    mutable std::mutex m_Mutex;
    std::thread own;

    bool should_work{};

    std::vector<Item> m_Items;
};

#endif