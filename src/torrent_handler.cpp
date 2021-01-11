#include "torrent_handler.h"
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/write_resume_data.hpp>
#include "item_window.h"
#include "resource_manager.h"
#include "settings_manager.h"
#include "formatter.h"
#include "logger.h"
#include "container.h"
#include <mutex>
#include <thread>
#include <chrono>
#include <filesystem>
#include <iostream>

namespace {

    using clk = std::chrono::steady_clock;

} // anonymous namespace

// return the name of a torrent status enum
std::string TorrentHandler::state(const lt::torrent_status &status) {
    bool IsPaused = (status.flags & (lt::torrent_flags::paused)) == lt::torrent_flags::paused;
    if (IsPaused) return "Paused";
    switch (status.state) {
        case lt::torrent_status::checking_files:
            return "Checking";
        case lt::torrent_status::downloading_metadata:
            return "Dl metadata";
        case lt::torrent_status::downloading:
            return "Downloading";
        case lt::torrent_status::finished:
            return "Finished";
        case lt::torrent_status::seeding:
            return "Seeding";
        case lt::torrent_status::checking_resume_data:
            return "Checking resume";
        default:
            return "<>";
    }
}


TorrentHandler::TorrentHandler() {
    lt::settings_pack p;
    p.set_int(lt::settings_pack::alert_mask, lt::alert_category::error
                                             | lt::alert_category::storage
                                             | lt::alert_category::status);
    _ses.apply_settings(p);

    own_work = std::thread([this] { do_work(); });
}

TorrentHandler::~TorrentHandler() {
    //std::lock_guard<std::mutex> lock(m_Mutex);

    signal_stop();
    own_work.detach();
//    if(own_work.joinable()) {
//        own_work.join();
//    }

//    for (auto &pair : m_Handles) {
//        if(pair.second.is_valid())
//            _ses.remove_torrent(pair.second);
//    }
//    for (auto &pair : m_Threads) {
//        pair.second.detach();
//    }
//    m_Threads.clear();
//    m_Handles.clear();
}

void TorrentHandler::AddTorrent(const std::string &url, const std::string &file_path) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    size_t curr_count = thread_count;
    m_Threads.insert(std::make_pair(curr_count, std::thread(
            [this, url, file_path, curr_count] { TorrentHandler::setup_torrent(url, file_path, curr_count); })));
    thread_count++;
}

void TorrentHandler::RemoveTorrent(const std::string &name, bool remove_files) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto handle = m_Handles[name];
    Logger::watcher w("Removing torrent" + handle.status().name);
    m_Handles.erase(m_Handles.find(name));
    if (remove_files) {
        _ses.remove_torrent(handle, lt::session_handle::delete_files);
    } else _ses.remove_torrent(handle);
}

void TorrentHandler::update_limits() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (auto &pair : m_Handles) {
        pair.second.set_download_limit(SettingsManager::get_settings().dl_limit * Formatter::MEGABYTE);
        pair.second.set_upload_limit(SettingsManager::get_settings().ul_limit * Formatter::MEGABYTE);
    }
}

int TorrentHandler::subscribe(const std::function<void()> &callback) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    int id = sub_count++;
    m_Callbacks.insert(std::make_pair(id, callback));
    return id;
}

void TorrentHandler::unsubscribe(int id) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Callbacks.erase(id);
}

void TorrentHandler::do_work() {

    should_work = true;
    clk::time_point last_save_resume = clk::now();

    while (should_work) {
        {
            std::lock_guard<std::mutex> lock(m_Mutex);

            //Logger::watcher w("Doing torrent check");

            std::vector<lt::alert *> alerts;
            _ses.pop_alerts(&alerts);

            for (auto &pair : m_Handles) {
                if (!pair.second.is_valid()) {
                    m_Handles.erase(m_Handles.find(pair.first));
                }
            }

            for (lt::alert const *a : alerts) {
                if (auto al = lt::alert_cast<lt::torrent_finished_alert>(a)) {
                    auto handle = al->handle;
                    if (!handle.is_valid()) continue;
                    handle.save_resume_data(lt::torrent_handle::save_info_dict);
                    for (auto &cb : m_CompletedCallbacks) {
                        cb(handle.status());
                    }
                }
                if (auto al = lt::alert_cast<lt::torrent_error_alert>(a)) {
                    if (!al->handle.is_valid()) continue;
                    al->handle.save_resume_data(lt::torrent_handle::save_info_dict);
                }
                if (auto al = lt::alert_cast<lt::add_torrent_alert>(a)) {
                    if (!al->handle.is_valid()) continue;
                    m_Handles.insert(std::make_pair(al->handle.status().name, al->handle));
                    for (auto &cb : m_AddedCallbacks) {
                        cb.second();
                    }
                }
                if (auto rd = lt::alert_cast<lt::save_resume_data_alert>(a)) {
                    if (!rd->handle.is_valid()) continue;
                    std::ofstream of(ResourceManager::get_torrent_save_dir(true) + (rd->handle.status().name),
                                     std::ios_base::binary);
                    of.unsetf(std::ios_base::skipws);
                    auto const b = write_resume_data_buf(rd->params);
                    of.write(b.data(), int(b.size()));
                }
            }
            _ses.post_torrent_updates();

            if (clk::now() - last_save_resume > std::chrono::seconds(3)) {
                for (auto &handle : _ses.get_torrents()) {
                    if (!handle.is_valid()) continue;
                    handle.save_resume_data(lt::torrent_handle::save_info_dict);
                    last_save_resume = clk::now();
                }
            }
        }
        //std::lock_guard<std::mutex> lock(m_Mutex);

        for (auto &cb : m_Callbacks) {
            cb.second();
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void TorrentHandler::signal_stop() {
    //std::lock_guard<std::mutex> lock(m_Mutex);
    should_work = false;

}

int TorrentHandler::subscribe_for_added(const std::function<void()> &callback) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    int id = sub_count++;
    m_AddedCallbacks.insert(std::make_pair(id, callback));
    return id;
}

void TorrentHandler::unsubscribe_from_added(int id) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_AddedCallbacks.erase(id);
}

void TorrentHandler::setup_torrent(const std::string &url, const std::string &file_path, size_t curr_count) {
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        Logger::watcher w("Torrent setup: " + url);

        lt::add_torrent_params params;

        if (url.rfind("magnet:?", 0) == 0) {
            params = lt::parse_magnet_uri(url);
        } else if (url.rfind("https://") == 0) {
            Logger::watcher watch("HTTPS link parse");
            std::string buffer;
            auto curl = curl_easy_init();
            if (curl) {
                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_HEADER, 0);
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,
                                 0); /* Don't follow anything else than the particular url requested*/
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                                 &Feed::writer);    /* Function Pointer "writer" manages the required buffer size */
                curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                                 &buffer); /* Data Pointer &buffer stores downloaded web content */
                curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
            } else {
                return;
            }
            curl_easy_perform(curl);
            curl_easy_cleanup(curl);

            try {
                lt::torrent_info info(buffer.c_str(), buffer.size());
                params = lt::parse_magnet_uri(lt::make_magnet_uri(info));
            } catch (...) {
                Logger::error("Failed to add torrent: " + url);
                return;
            }
        } else {
            try {
                lt::torrent_info info(url);
                params = lt::parse_magnet_uri(lt::make_magnet_uri(info));
            } catch (...) {
                Logger::error("Failed to add torrent: " + url);
                return;
            }
        }

        for (const auto &entry : std::filesystem::directory_iterator(ResourceManager::get_torrent_save_dir())) {
            try {
                std::ifstream ifs(entry.path(), std::ios_base::binary);
                ifs.unsetf(std::ios_base::skipws);
                std::vector<char> buf{std::istream_iterator<char>(ifs), std::istream_iterator<char>()};
                if (!buf.empty()) {
                    auto atp = lt::read_resume_data(buf);
                    if (atp.info_hash == params.info_hash) {
                        params = std::move(atp);
                        break;
                    }
                }
            }
            catch (std::exception &ex) {
                Logger::error(ex.what());
                std::filesystem::remove(entry);
            }
        }
        bool add = true;
        for (auto &download : DataContainer::get_downloaded()) {
            if (params.name == download) {
                add = false;
                break;
            }
        }
        if (add)
            DataContainer::add_downloaded(params.name);
        params.download_limit = SettingsManager::get_settings().dl_limit * Formatter::MEGABYTE;
        params.upload_limit = SettingsManager::get_settings().ul_limit * Formatter::MEGABYTE;
        params.save_path = file_path;
        _ses.async_add_torrent(std::move(params));
        m_Threads[curr_count].detach();
        m_Threads.erase(curr_count);
    }
}

void TorrentHandler::subscribe_for_completed(const std::function<void(const lt::torrent_status &)> &callback) {
    m_CompletedCallbacks.push_back(callback);
}
