#pragma once

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>
namespace algorithm {
    template<typename T>
class RingBuffer {
    public:
        explicit RingBuffer(size_t capacity)
            : capacity_(capacity) {}
    
        void push(T value) {
            if (data_.size() < capacity_) {
                data_.push_back(std::move(value));
            } else {
                data_[head_] = std::move(value);
                head_ = (head_ + 1) % capacity_;
            }
        }
    
        std::vector<T> values() const {
            std::vector<T> result;
        
            if (data_.size() < capacity_) {
                return data_;
            }
        
            result.reserve(data_.size());
        
            for (size_t i = 0; i < data_.size(); ++i) {
                size_t index = (head_ + i) % data_.size();
                result.push_back(data_[index]);
            }
        
            return result;
        }

        size_t size() const {
            return data_.size();
        }

        std::optional<T> last() const {
            if (data_.empty()) {
                return std::nullopt;
            }

            if (data_.size() < capacity_) {
                return data_.back();
            } else {
                size_t last_index = (head_ == 0) ? capacity_ - 1 : head_ - 1;
                return data_[last_index];
            }
        }
    
    private:
        size_t capacity_;
        size_t head_ = 0;
        std::vector<T> data_;
    };
}