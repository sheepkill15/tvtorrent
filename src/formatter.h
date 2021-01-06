#ifndef TVTORRENT_FORMATTER_H
#define TVTORRENT_FORMATTER_H

#include "glibmm/ustring.h"
#include <iomanip>

class Formatter {
    public:
    
    static Glib::ustring format_size(int64_t bytes) {
		if(bytes <= KILOBYTE) {
			return Glib::ustring::format(bytes) + " B";
		}
		if(bytes <= MEGABYTE) {
			return Glib::ustring::format(std::fixed, std::setprecision(2), bytes / KILOBYTE) + " KB";
		}
		if(bytes <= GIGABYTE) {
			return Glib::ustring::format(std::fixed, std::setprecision(2), bytes / (MEGABYTE)) + " MB";
		}
		if(bytes <= TERRABYTE) {
			return Glib::ustring::format(std::fixed, std::setprecision(2), bytes / (GIGABYTE)) + " GB";
		}
		return Glib::ustring::format(std::fixed, std::setprecision(2), bytes / (TERRABYTE)) + " TB";
	}

    static Glib::ustring format_time(int64_t seconds) {
        if(seconds <= MINUTE) {
            return Glib::ustring::format(seconds) + " s";
        }
        if(seconds <= HOUR) {
            return Glib::ustring::format(seconds / MINUTE, " min ", seconds % MINUTE, " s");
        }
        if(seconds <= DAY) {
            return Glib::ustring::format(seconds / HOUR, " h ", seconds % HOUR / MINUTE, " min ", seconds % MINUTE, " s");
        }
        if(seconds <= WEEK) {
            return Glib::ustring::format(seconds / DAY, " d ", seconds % DAY / HOUR, " h ", seconds % HOUR / MINUTE, " min ");
        }
        if(seconds / WEEK >= 4)
            return Glib::ustring::format(seconds / WEEK, " w ") + Glib::ustring::format(seconds % WEEK / DAY, " d ");
        return Glib::ustring::format(seconds / WEEK, " w ") + Glib::ustring::format(seconds % WEEK / DAY, " d ", seconds % DAY / HOUR, " h ");
    }

    constexpr const static float KILOBYTE = 1024;
	constexpr const static float MEGABYTE = 1024*1024;
	constexpr const static float GIGABYTE = 1024.F*1024*1024;
	constexpr const static float TERRABYTE = 1024.F*1024*1024*1024;

    constexpr const static int MINUTE = 60;
    constexpr const static int HOUR = 60*60;
    constexpr const static int DAY = 60*60*24;
    constexpr const static int WEEK = 60*60*24*7;
};

#endif