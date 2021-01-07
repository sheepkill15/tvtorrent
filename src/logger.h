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

    static void init();

    static void info(const std::string&);
    static void debug(const std::string&);
    static void error(const std::string&);
    static void warning(const std::string&);

private:
    inline static std::ofstream logfile; // NOLINT(cert-err58-cpp)
};


#endif //TVTORRENT_LOGGER_H
