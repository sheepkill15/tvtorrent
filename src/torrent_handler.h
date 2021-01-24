#ifndef TVTORRENT_TORRENT_HANDLER_H
#define TVTORRENT_TORRENT_HANDLER_H

#include "tv_item.h"
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_status.hpp>
#include <mutex>

class TTItemWindow;

class TorrentHandler {
	
	public:
		TorrentHandler();
		virtual ~TorrentHandler();

		void AddTorrent(const std::string& url, const std::string& file_path);
		void RemoveTorrent(size_t, bool);
		void do_work();
		void signal_stop();
        void update_limits();
		int subscribe(const std::function<void()>& callback);
		int subscribe_for_added(const std::function<void()>& callback);
		void subscribe_for_completed(const std::function<void(const lt::torrent_handle&)>& callback);
		void unsubscribe(int id);
		void unsubscribe_from_added(int id);

		static std::string state(const lt::torrent_status& status);

		std::unordered_map<size_t, lt::torrent_handle> m_Handles;
	private:
        size_t sub_count = 0;
        size_t thread_count = 0;

		lt::session _ses;
		bool should_work = true;
		std::thread* own_work;

		std::unordered_map<int, std::function<void()>> m_Callbacks;
		std::unordered_map<int, std::function<void()>> m_AddedCallbacks;
		std::vector<std::function<void(const lt::torrent_handle&)>> m_CompletedCallbacks;
		std::unordered_map<int, std::thread> m_Threads;

		mutable std::mutex m_Mutex;

		void setup_torrent(const std::string& url, const std::string& file_path, size_t curr_count);
};

#endif
