//
// Created by simon on 2021. jan. 7..
//

#ifndef TVTORRENT_LOGGER_H
#define TVTORRENT_LOGGER_H

#include <iostream>
#include <fstream>

class Logger {
public:
    Logger() = delete;
    ~Logger() = default;

    class watcher {
    public:
        explicit watcher(const std::string& text);
        ~watcher();

    private:
        std::string m_Text;
    };

    static void init();
    static void cleanup();

    static void info(const std::string&);
    static void debug(const std::string&);
    static void error(const std::string&);
    static void warning(const std::string&);

private:
    inline static std::ofstream logfile; // NOLINT(cert-err58-cpp)
    inline static std::streambuf *coutbuf;
};


#endif //TVTORRENT_LOGGER_H
