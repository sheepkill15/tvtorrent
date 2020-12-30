#include "torrent_handler.h"
#include <fstream>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/session_settings.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/write_resume_data.hpp>
#include "item_window.h"
#include "resource_manager.h"
#include <mutex>
#include <thread>
#include <chrono>
#include <iostream>
#include <filesystem>
#include <utility>

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

TorrentHandler::~TorrentHandler() = default;

lt::torrent_handle TorrentHandler::AddTorrent(const std::string &url, const std::string& file_path)
{

	std :: cout << file_path << std :: endl;

	lt::add_torrent_params params;

	if(url.rfind("magnet:?", 0) == 0 || url.rfind("https://") == 0) {
		params = lt::parse_magnet_uri(url);
	}
	else {
		lt::torrent_info info(url);
		params = lt::parse_magnet_uri(lt::make_magnet_uri(info));
		std :: cout << "Processed file!" << std::endl;
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
	params.download_limit = 7000000;
	params.upload_limit = 7000000;
	params.save_path = file_path;
	auto handle = _ses.add_torrent(std::move(params));
	m_Handles.insert(std::make_pair(handle.status().name, handle));
	return handle;
}

void TorrentHandler::RemoveTorrent(const std::string& name) {
	std::lock_guard<std::mutex> lock(m_Mutex);
    m_Handles[name].pause();
    auto handle = m_Handles[name];
    handle.flush_cache();
	_ses.remove_torrent(handle);
	m_Handles.erase(m_Handles.find(name));
	while(handle.is_valid()) ;
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
				handle.save_resume_data(lt::torrent_handle::save_info_dict);
			}
			if(auto al = lt::alert_cast<lt::torrent_error_alert>(a)) {
				std::cout << al->message() << std::endl;
				al->handle.save_resume_data(lt::torrent_handle::save_info_dict);
			}
			if(auto rd = lt::alert_cast<lt::save_resume_data_alert>(a)) {
				std::ofstream of(ResourceManager::get_torrent_save_dir(true) + (rd->handle.status().name), std::ios_base::binary);
				of.unsetf(std::ios_base::skipws);
				auto const b = write_resume_data_buf(rd->params);
				of.write(b.data(), int(b.size()));
			}
		}
		_ses.post_torrent_updates();

		if(clk::now() - last_save_resume > std::chrono::seconds(3)) {
			for(auto& handle : _ses.get_torrents()) {
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
