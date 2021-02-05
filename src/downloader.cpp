//
// Created by simon on 2021. febr. 5..
//

#include "downloader.h"

#include <curl/curl.h>

size_t Downloader::writer(char *data, size_t size, size_t nmemb, std::string *buffer){
    size_t result = 0;
    if(buffer != nullptr) {
        buffer -> append(data, size * nmemb);
        result = size * nmemb;
    }
    return result;
}

void Downloader::fetch_no_alloc(const std::string &link, std::string &buffer) {
    auto curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, link.c_str());
        curl_easy_setopt(curl, CURLOPT_HEADER, 0);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,
                         0); /* Don't follow anything else than the particular url requested*/
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                         &Downloader::writer);    /* Function Pointer "writer" manages the required buffer size */
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer); /* Data Pointer &buffer stores downloaded web content */
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "TVTorrent");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30);
    } else {
        return;
    }
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
}
