#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <boost/thread/shared_mutex.hpp>
#include <boost/functional/hash.hpp>
#include <boost/thread/mutex.hpp>
#include <functional>
#include <algorithm>
#include <atomic>
#include <memory>
#include <vector>
#include <mutex>
#include <list>

namespace lock_based {

template <typename Key, typename Value, typename Hash = boost::hash<Key> >
class hash_map {
private:
    static size_t next_buckets_counts[];

    enum { MAX_LOAD_FACTOR = 2 };

    struct bucket_type;

public:
    typedef Key key_type;
    typedef Value mapped_type;
    typedef std::pair<const Key, Value> value_type;

    hash_map();

    /*! \brief Get a value associated with the key. Return true if found. */
    bool get(const Key & key, Value & value) const;

    /*!
        \brief Add key/value pair to the table. Return true if a new element was
               added, false if an existing element was updated.
    */
    bool insert(const value_type & value);

    /*!
        \brief Add key/value to the table. Return true if a new element was
               added, false if an existing element was updated.
    */
    bool insert(const Key & key, const Value & value);

    /*! \brief Remove key/value associated with the key. Return true if item was indeed removed. */
    bool remove(const Key & key);

    /*! \brief Remove all elements from the table container. */
    void clear();

    /*! \brief Return number of elements in table. */
    size_t size() const;

private:
    /*! \brief Resize table to a new size and copy over existing elements. */
    void resize(size_t new_buckets_count);

    size_t get_next_buckets_count() const;

    size_t get_bucket_index(const Key & key) const;

    const bucket_type & get_bucket(const Key & key) const;

    bucket_type & get_bucket(const Key & key);

    double get_load_factor() const;

    Hash m_hasher;

    // TODO
    std::atomic<int> m_size;

    mutable int m_next_buckets_count_index;

    std::vector<bucket_type> m_buckets;
    mutable boost::shared_mutex m_mutex;
};

template <typename Key, typename Value, typename Hash>
size_t hash_map<Key, Value, Hash>::next_buckets_counts[] = {
    1,
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

template <typename Key, typename Value, typename Hash>
struct hash_map<Key, Value, Hash>::bucket_type {
    bucket_type() : m_mutex(new boost::shared_mutex) { }

    bool get(const Key & key, Value & value) const {
        auto it = std::find_if(
                m_data.begin(),
                m_data.end(),
                [&](const value_type & item){ return item.first == key; });

        if (it == m_data.end()) {
            return false;
        }

        value = it->second;
        return true;
    }

    bool insert(const Key & key, const Value & value) {
        auto it = std::find_if(
                m_data.begin(),
                m_data.end(),
                [&](const value_type & item){ return item.first == key; });

        if (it != m_data.end()) {
            it->second = value;
            return false;
        } else {
            m_data.push_back(value_type(key, value));
            return true;
        }
    }

    bool remove(const Key & key) {
        auto it = std::find_if(
                m_data.begin(),
                m_data.end(),
                [&](const value_type & item){ return item.first == key; });

        if (it != m_data.end()) {
            m_data.erase(it);
            return true;
        }

        return false;
    }

    std::list<value_type> m_data;
    std::unique_ptr<boost::shared_mutex> m_mutex;
};

template <typename Key, typename Value, typename Hash>
hash_map<Key, Value, Hash>::hash_map() : m_size(0), m_next_buckets_count_index(0) { }

template <typename Key, typename Value, typename Hash>
bool hash_map<Key, Value, Hash>::get(const Key & key, Value & value) const {
    boost::shared_lock<boost::shared_mutex> table_lock(m_mutex);

    const bucket_type & bucket = get_bucket(key);

    boost::shared_lock<boost::shared_mutex> bucket_lock(*bucket.m_mutex);
    return bucket.get(key, value);
}

template <typename Key, typename Value, typename Hash>
bool hash_map<Key, Value, Hash>::insert(const value_type & value) {
    return insert(value.first, value.second);
}

template <typename Key, typename Value, typename Hash>
bool hash_map<Key, Value, Hash>::insert(const Key & key, const Value & value) {
    bool can_insert = false;
    do {
        {
            // Check if a resize is needed
            boost::upgrade_lock<boost::shared_mutex> lock(m_mutex);

            if (MAX_LOAD_FACTOR < get_load_factor()) {
                // Get new size
                size_t new_buckets_count = get_next_buckets_count();

                boost::upgrade_to_unique_lock<boost::shared_mutex> upgrade_lock(lock);
                // Create new bukets and copy over existing values
                resize(new_buckets_count);
            }
        }

        boost::shared_lock<boost::shared_mutex> table_lock(m_mutex);
        if (MAX_LOAD_FACTOR < get_load_factor()) {
            continue;
        }

        can_insert = true;

        bucket_type & bucket = get_bucket(key);

        boost::unique_lock<boost::shared_mutex> bucket_lock(*bucket.m_mutex);
        if (bucket.insert(key, value)) {
            ++m_size;
        }
    } while (!can_insert);

    return true;
}

template <typename Key, typename Value, typename Hash>
bool hash_map<Key, Value, Hash>::remove(const Key & key) {
    boost::shared_lock<boost::shared_mutex> table_lock(m_mutex);

    bucket_type & bucket = get_bucket(key);

    boost::unique_lock<boost::shared_mutex> bucket_lock(*bucket.m_mutex);
    if (bucket.remove(key)) {
        --m_size;
        return true;
    }

    return false;
}

template <typename Key, typename Value, typename Hash>
void hash_map<Key, Value, Hash>::clear() {
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);
    m_buckets.clear();
    m_size = 0;
}

template <typename Key, typename Value, typename Hash>
size_t hash_map<Key, Value, Hash>::size() const {
    return m_size;
}

template <typename Key, typename Value, typename Hash>
void hash_map<Key, Value, Hash>::resize(size_t new_buckets_count) {
    std::vector<bucket_type> new_buckets(new_buckets_count);

    for (size_t i = 0; i < m_buckets.size(); ++i) {
        for (auto it = m_buckets[i].m_data.begin(); it != m_buckets[i].m_data.end(); ) {
            size_t bucket_index = m_hasher(it->first) % new_buckets.size();
            new_buckets[bucket_index].m_data.push_back(std::move(*(it++)));
        }
    }

    m_buckets.swap(new_buckets);
}

template <typename Key, typename Value, typename Hash>
size_t hash_map<Key, Value, Hash>::get_next_buckets_count() const {
    size_t buckets_count = next_buckets_counts[m_next_buckets_count_index];
    ++m_next_buckets_count_index;
    return buckets_count;
}

template <typename Key, typename Value, typename Hash>
size_t hash_map<Key, Value, Hash>::get_bucket_index(const Key & key) const {
    return m_hasher(key) % m_buckets.size();
}

template <typename Key, typename Value, typename Hash>
const typename hash_map<Key, Value, Hash>::bucket_type & hash_map<Key, Value, Hash>::get_bucket(const Key & key) const {
    size_t bucket_index = m_hasher(key) % m_buckets.size();
    return m_buckets[bucket_index];
}

template <typename Key, typename Value, typename Hash>
typename hash_map<Key, Value, Hash>::bucket_type & hash_map<Key, Value, Hash>::get_bucket(const Key & key) {
    size_t bucket_index = m_hasher(key) % m_buckets.size();
    return m_buckets[bucket_index];
}

template <typename Key, typename Value, typename Hash>
double hash_map<Key, Value, Hash>::get_load_factor() const {
    if (!m_buckets.empty()) {
        return static_cast<double>(m_size.load()) / m_buckets.size();
    } else {
        return MAX_LOAD_FACTOR + 1;
    }
}

} // namespace lock_based

#endif
