#pragma once
#include <functional>

// Combine hash values
inline void hash_combine(std::size_t &seed, const std::size_t &mask, const std::size_t &hash)
{
  seed ^= hash + mask + (seed << 6) + (seed >> 2);
};

// Combine hash values
template <typename T>
inline void hash_combine(std::size_t &seed, const std::size_t &mask, const T &v)
{
  seed ^= std::hash<T>()(v) + mask + (seed << 6) + (seed >> 2);
}
