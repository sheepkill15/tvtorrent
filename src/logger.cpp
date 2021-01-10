//
// Created by simon on 2021. jan. 7..
//

#include "logger.h"
#include <filesystem>
#include <chrono>

using clk = std::chrono::system_clock;

namespace {


    std::string LOG_DIRECTORY;
    void create_if_doesnt_exist(const std::string& path) {
        if(!std::filesystem::is_directory(path) || !std::filesystem::exists(path)) {
            std::filesystem::create_directory(path);
        }
    }
    std::string formatted_now() {
        std::stringstream ss;
        auto now = clk::to_time_t(clk::now());
        ss << '[' << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M") << ']';
        return ss.str();
    }
}

#if defined(_WIN64) || defined(_WIN32) // Windows platform specific

namespace {
    constexpr const char DELIM = '\\';
}

void Logger::init() {
    LOG_DIRECTORY = std::string(getenv("APPDATA")) + "\\TVTorrent\\logs";
    create_if_doesnt_exist(LOG_DIRECTORY);
    std::stringstream ss;
    auto now = clk::to_time_t(clk::now());
    ss << std::put_time(std::localtime(&now), "%Y-%m-%d_%H-%M-%S") << ".log";
    logfile = std::ofstream(LOG_DIRECTORY + DELIM + ss.str(), std::ios::out | std::ios::trunc);
    if(logfile.fail()) {
        std::cerr << "Log file couldn't be instantiated!" << std::endl;
        return;
    }
    coutbuf = std::cout.rdbuf();
    std::cout.rdbuf(logfile.rdbuf());
    std::cerr.rdbuf(logfile.rdbuf());
    std::clog.rdbuf(logfile.rdbuf());

    info("Initialized logging!");
}

#else // Linux platform specific

namespace {
    static std::string SAVE_DIRECTORY;
    constexpr const char DELIM = '/';
}

void Logger::init() {
	LOG_DIRECTORY = std::string(getenv("HOME")) + "/.local/TVTorrent/logs";
    create_if_doesnt_exist(LOG_DIRECTORY);

    logfile = std::ofstream(LOG_DIRECTORY + DELIM + "latest.log", std::ios::out | std::ios::app);
    if(logfile.fail()) {
        error("Log file couldn't be instantiated!");
        return;
    }
    coutbuf = std::cout.rdbuf();
    logfile << coutbuf;
    std::cout.rdbuf(logfile.rdbuf());
    info("Initialized logging!");
}

#endif
void Logger::info(const std::string &info) {
    logfile << formatted_now() << " i: " << info << std::endl;
}

void Logger::debug(const std::string &info) {
    logfile << formatted_now() << " d: " << info << std::endl;

}

void Logger::error(const std::string &info) {
    logfile << formatted_now() << " e: " << info << std::endl;

}

void Logger::warning(const std::string &info) {
    logfile << formatted_now() << " w: " << info << std::endl;
}

void Logger::cleanup() {

    std::cout.rdbuf(coutbuf);

    info("Cleanup complete");
    logfile.close();
}

Logger::watcher::watcher(const std::string &text)
    :m_Text(text)
{
    Logger::info("Watcher start: " + text);
}

Logger::watcher::~watcher() {
    Logger::info("Watcher end: " + m_Text);
}
