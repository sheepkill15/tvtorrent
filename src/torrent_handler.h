#ifndef TVTORRENT_TORRENT_HANDLER_H
#define TVTORRENT_TORRENT_HANDLER_H

#include "tv_item.h"
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_status.hpp>
#include <glibmm/ustring.h>
#include <mutex>

class TTItemWindow;

class TorrentHandler {
	
	public:
		TorrentHandler();
		virtual ~TorrentHandler();

		lt::torrent_handle AddTorrent(const Glib::ustring& url, const Glib::ustring& file_path);
		void RemoveTorrent(const Glib::ustring& name);
		std::vector<lt::torrent_status const * > FetchTorrentUpdates();
		void do_work();
		void signal_stop();

		int subscribe(const std::function<void()>& callback);
		void unsubscribe(int id);

		static std::string state(lt::torrent_status status);

		std::unordered_map<std::string, lt::torrent_handle> m_Handles;
	private:
		lt::session _ses;
		bool should_work = true;

		std::unordered_map<int, std::function<void()>> m_Callbacks;

		mutable std::mutex m_Mutex;
		
};

#endif
