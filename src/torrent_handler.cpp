#include "torrent_handler.h"
#include <fstream>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/write_resume_data.hpp>
#include "item_window.h"
#include "resource_manager.h"
#include "settings_manager.h"
#include "main_window.h"
#include "formatter.h"
#include <mutex>
#include <thread>
#include <chrono>
#include <filesystem>

namespace {

using clk = std::chrono::steady_clock;

} // anonymous namespace

// return the name of a torrent status enum
std::string TorrentHandler::state(const lt::torrent_status& status)
{
  bool IsPaused = (status.flags & (lt::torrent_flags::paused)) == lt::torrent_flags::paused;
  if(IsPaused) return "Paused";
  switch(status.state) {
    case lt::torrent_status::checking_files: return "Checking";
    case lt::torrent_status::downloading_metadata: return "Dl metadata";
    case lt::torrent_status::downloading: return "Downloading";
    case lt::torrent_status::finished: return "Finished";
    case lt::torrent_status::seeding: return "Seeding";
    case lt::torrent_status::checking_resume_data: return "Checking resume";
    default: return "<>";
  }
}


TorrentHandler::TorrentHandler()
{
	lt::settings_pack p;
	p.set_int(lt::settings_pack::alert_mask
    , lt::alert_category::error
    | lt::alert_category::storage
    | lt::alert_category::status);
	_ses.apply_settings(p);
}

TorrentHandler::~TorrentHandler() {
    for(auto& pair : m_Handles) {
        _ses.remove_torrent(pair.second);
    }
    m_Handles.clear();
}

lt::torrent_handle TorrentHandler::AddTorrent(const std::string &url, const std::string& file_path)
{

	lt::add_torrent_params params;

	if(url.rfind("magnet:?", 0) == 0) {
		params = lt::parse_magnet_uri(url);
	}
	else if(url.rfind("https://") == 0) {
	    std::string buffer;
        auto curl = curl_easy_init();
        if(curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_HEADER, 0);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0); /* Don't follow anything else than the particular url requested*/
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &Feed::writer);	/* Function Pointer "writer" manages the required buffer size */
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer ); /* Data Pointer &buffer stores downloaded web content */
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        } else {
            return lt::torrent_handle();
        }
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        lt::torrent_info info(buffer.c_str(), buffer.size());
        params = lt::parse_magnet_uri(lt::make_magnet_uri(info));
	}
	else {
		lt::torrent_info info(url);
		params = lt::parse_magnet_uri(lt::make_magnet_uri(info));
	}
	
	for(const auto& entry : std::filesystem::directory_iterator(ResourceManager::get_torrent_save_dir())) {
		std::ifstream ifs(entry.path(), std::ios_base::binary);
		ifs.unsetf(std::ios_base::skipws);
		std::vector<char> buf{std::istream_iterator<char>(ifs), std::istream_iterator<char>()};
		if(!buf.empty()) {
			auto atp = lt::read_resume_data(buf);
			if(atp.info_hash == params.info_hash) {params = std::move(atp);
				break;
			}
		}
	}
	bool add = true;
	for(auto& download : TTMainWindow::m_Downloaded) {
	    if(params.name == download) {
	        add = false;
	        break;
	    }
	}
	if(add)
        TTMainWindow::m_Downloaded.push_back(params.name);
	params.download_limit = SettingsManager::get_settings().dl_limit * Formatter::MEGABYTE;
	params.upload_limit = SettingsManager::get_settings().ul_limit * Formatter::MEGABYTE;
	params.save_path = file_path;
	auto handle = _ses.add_torrent(std::move(params));
	m_Handles.insert(std::make_pair(handle.status().name, handle));
	return handle;
}

void TorrentHandler::RemoveTorrent(const std::string& name, bool remove_files) {
	std::lock_guard<std::mutex> lock(m_Mutex);
    m_Handles[name].pause();
    auto handle = m_Handles[name];
    m_Handles.erase(m_Handles.find(name));
    handle.flush_cache();
    if(remove_files)
	    _ses.remove_torrent(handle, lt::session_handle::delete_files);
    else _ses.remove_torrent(handle);
}

void TorrentHandler::update_limits() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for(auto& pair : m_Handles) {
        pair.second.set_download_limit(SettingsManager::get_settings().dl_limit * Formatter::MEGABYTE);
        pair.second.set_upload_limit(SettingsManager::get_settings().ul_limit * Formatter::MEGABYTE);
    }
}

int TorrentHandler::subscribe(const std::function<void()>& callback) {
	//std::lock_guard<std::mutex> lock(m_Mutex);
	int id = sub_count++;
	m_Callbacks.insert(std::make_pair(id, callback));
	return id;
}

void TorrentHandler::unsubscribe(int id) {
	//std::lock_guard<std::mutex> lock(m_Mutex);
	m_Callbacks.erase(id);
}

void TorrentHandler::do_work() {

	should_work = true;
	clk::time_point last_save_resume = clk::now();

	while(should_work) {
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		std::vector<lt::alert*> alerts;
		_ses.pop_alerts(&alerts);

		for(lt::alert const* a : alerts) {
			if(auto al = lt::alert_cast<lt::torrent_finished_alert>(a)) {
				auto handle = al->handle;
				if(!handle.is_valid()) continue;
				handle.save_resume_data(lt::torrent_handle::save_info_dict);
			}
			if(auto al = lt::alert_cast<lt::torrent_error_alert>(a)) {
                if(!al->handle.is_valid()) continue;
				al->handle.save_resume_data(lt::torrent_handle::save_info_dict);
			}
			if(auto rd = lt::alert_cast<lt::save_resume_data_alert>(a)) {
                if(!rd->handle.is_valid()) continue;
				std::ofstream of(ResourceManager::get_torrent_save_dir(true) + (rd->handle.status().name), std::ios_base::binary);
				of.unsetf(std::ios_base::skipws);
				auto const b = write_resume_data_buf(rd->params);
				of.write(b.data(), int(b.size()));
			}
		}
		_ses.post_torrent_updates();

		if(clk::now() - last_save_resume > std::chrono::seconds(3)) {
			for(auto& handle : _ses.get_torrents()) {
			    if(!handle.is_valid()) continue;
				handle.save_resume_data(lt::torrent_handle::save_info_dict);
				last_save_resume = clk::now();
			}
		}
	}	
		//std::lock_guard<std::mutex> lock(m_Mutex);

		for(auto& cb : m_Callbacks) {
			cb.second();
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

void TorrentHandler::signal_stop() { 
	std::lock_guard<std::mutex> lock(m_Mutex);
	should_work = false; 

}
