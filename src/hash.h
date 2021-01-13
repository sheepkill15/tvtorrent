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
//        auto found = m_Cache.find(myString);
//        if(found != m_Cache.end()) {
//            return found->second;
//        }
//        size_t randomizer = 100;
//        for (char i : myString)
//        {
//            randomizer += i;
//        }
//        randomizer += myString.size() * myString[0];
//        randomizer += myString.back();
//        randomizer *= myString.back();

        //m_Cache.insert(std::make_pair(myString, randomizer));
        return m_Hasher(myString);
    }
private:
    /// Caching
    inline static std::hash<std::string> m_Hasher;
    //inline static std::unordered_map<std::string, size_t> m_Cache;
};

#endif //TVTORRENT_HASH_H
