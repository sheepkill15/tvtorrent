//
// Created by simon on 2021. jan. 24..
//

#ifndef TVTORRENT_UPDATER_H
#define TVTORRENT_UPDATER_H

#include <glibmm/dispatcher.h>
#include <thread>

class Updater {
public:
    Updater() = delete;
    ~Updater() = default;

    static void Start();
    static void CheckUpdates();
    static void ForceFinish();

private:
    constexpr static const char tag[] = "v2.0f";

    static void CreateUpdateDialog();

    inline static std::thread own;

    inline static Glib::Dispatcher create_dialog;
    inline static std::string update_tag;

};


#endif //TVTORRENT_UPDATER_H
