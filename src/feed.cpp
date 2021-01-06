#include "feed.h"
#include <iostream>
#include <utility>

namespace {
    unsigned long long writer(char *data, size_t size, size_t nmemb, std::string *buffer){
    	unsigned long long result = 0;
    	if(buffer != nullptr) {
    		buffer -> append(data, size * nmemb);
    		result = size * nmemb;
    	}
    	return result;
    } 
}

Feed::Feed(std::string  rss_url)
    :RSS_URL(std::move(rss_url))
{
    parse_feed(true);
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
            parse_feed();
        }
	    std::this_thread::sleep_for(std::chrono::minutes(5));
    }
}

void Feed::parse_feed(bool first) {
    
    buffer.clear();
    curl = curl_easy_init();
    if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, RSS_URL.c_str());
		curl_easy_setopt(curl, CURLOPT_HEADER, 0);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0); /* Don't follow anything else than the particular url requested*/
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);	/* Function Pointer "writer" manages the required buffer size */
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer ); /* Data Pointer &buffer stores downloaded web content */	
	} else {
		std::cerr << "Curl couldn't be configured! " << std::endl;
        return;
	}
	curl_easy_perform(curl);
	curl_easy_cleanup(curl);
    doc.clear();
	doc.parse<0>(buffer.data());
    auto root = doc.first_node();
    if(std::string(root->name()) != "rss") return;
    auto channel = root->first_node("channel");
  
    if(first) {
        auto title = channel->first_node("title");
        auto desc = channel->first_node("description");
        auto link = channel->first_node("link");
        channel_data.title = title->value();
        channel_data.desc = desc->value();
        channel_data.link = link->value();
    }
    m_Items.clear();
    for(auto child = channel->first_node("item"); child != nullptr; child = child->next_sibling("item")) {
        parse_item(child);
    }

//    for(auto& item : m_Items) {
//        std::cout << item.title << std::endl << item.link << std::endl << item.date << std::endl;
//    }

	//std::cout << "Name of first node is: " << doc.first_node()->name() << std::endl;
}

void Feed::parse_item (rapidxml::xml_node<>* child) {
    auto title = child->first_node("title");
    std::string title_value = title->value();

    m_Items.push_back({title_value, child->first_node("link")->value(), child->first_node("pubDate")->value()});
}