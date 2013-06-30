#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <functional>
#include <algorithm>
#include <atomic>
#include <memory>
#include <vector>
#include <mutex>
#include <list>

namespace lock_based {

template <typename Key, typename Value, typename Hash = std::hash<Key> >
class hash_map final {
private:
    static std::size_t sizes[];
#if 0
        1,
        2,
        3,
        7,
        13,
        31,
        61,
        127,
        251,
        509,
        1021,
        2039,
        4093,
        8191,
        16381,
        32749,
        65521,
        131071,
        262139,
        524287,
        1048573,
        2097143,
        4194301,
        8388593,
        16777213,
        33554393,
        67108859,
        134217689,
        268435399,
        536870909,
        1073741789,
        2147483647 };
#endif

    typedef std::pair<const Key, Value> bucket_value;

    typedef std::list<bucket_value> bucket_data;

    struct bucket_type {
        bool get(const Key & key, Value & value) const {
            auto it = std::find_if(
                m_data.begin(),
                m_data.end(),
                [&](const bucket_value & item){ return item.first == key; });

            if (it == m_data.end()) {
                return false;
            }

            value = it->second;
            return true;
        }

        void insert(const Key & key, const Value & value) {
            auto it = std::find_if(
                m_data.begin(),
                m_data.end(),
                [&](const bucket_value & item){ return item.first == key; });

            if (it != m_data.end()) {
                it->second = value;
            } else {
                m_data.push_back(bucket_value(key, value));
            }
        }

        bool remove(const Key & key) {
            auto it = std::find_if(
                m_data.begin(),
                m_data.end(),
                [&](const bucket_value & item){ return item.first == key; });

            if (it != m_data.end()) {
                m_data.erase(it);
                return true;
            }

            return false;
        }

        bucket_data m_data;
        mutable std::mutex m_mutex;
    };

public:
    typedef Key key_type;
    typedef Value mapped_type;
    typedef std::pair<const Key, Value> value_type;

    bool get(const Key & key, Value & value) const {
        const bucket_type & bucket = get_bucket(key);

        std::lock_guard<std::mutex> lock(bucket.m_mutex);
        return bucket.get(key, value);
    }

    void insert(const Key & key, const Value & value) {
        bucket_type & bucket = get_bucket(key);

        std::lock_guard<std::mutex> lock(bucket.m_mutex);
        bucket.insert(key, value);
    }

    bool remove(const Key & key) {
        bucket_type & bucket = get_bucket(key);

        std::lock_guard<std::mutex> lock(bucket.m_mutex);
        return bucket.remove(key);
    }

    std::size_t size() const {
        return m_size;
    }

private:
    std::size_t get_bucket_index(const Key & key) const {
        return m_hasher(key) % m_buckets.size();
    }

    const bucket_type & get_bucket(const Key & key) const {
        std::size_t bucket_index = m_hasher(key) % m_buckets.size();
        return m_buckets[bucket_index];
    }

    bucket_type & get_bucket(const Key & key) {
        std::size_t bucket_index = m_hasher(key) % m_buckets.size();
        return m_buckets[bucket_index];
    }

    Hash m_hasher;

    std::atomic<int> m_size;

    std::vector<bucket_type> m_buckets;
};

} // namespace lock_based

#endif
