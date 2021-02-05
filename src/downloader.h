//
// Created by simon on 2021. febr. 5..
//

#ifndef TVTORRENT_DOWNLOADER_H
#define TVTORRENT_DOWNLOADER_H

#include <string>

class Downloader {
public:
    Downloader() = delete;
    ~Downloader() = default;

    static void fetch_no_alloc(const std::string& link, std::string& buffer);

private:
    static size_t writer(char *data, size_t size, size_t nmemb, std::string *buffer);
};


#endif //TVTORRENT_DOWNLOADER_H
