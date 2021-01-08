//
// Created by simon on 2021. jan. 8..
//

#ifndef TVTORRENT_HASH_H
#define TVTORRENT_HASH_H

#include <string>

class Unique {
public:
    Unique() = delete;
    ~Unique() = default;
    inline static size_t from_string(const std::string& myString) {
        size_t randomizer = 100;
        for (char i : myString)
        {
            randomizer += i;
        }
        randomizer += myString.size() * myString[0];
        randomizer += myString.back();
        randomizer *= myString.back();
        return randomizer;
    }
};

#endif //TVTORRENT_HASH_H
