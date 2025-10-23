#pragma once

#include <functional>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace ecs {

class SystemEventBus {
public:
    struct SubscriptionToken {
        std::type_index type{typeid(void)};
        size_t id = 0;

        SubscriptionToken() = default;
        SubscriptionToken(std::type_index t, size_t identifier)
            : type(t), id(identifier) {}

        bool IsValid() const { return id != 0; }
    };

    template<typename Event, typename Func>
    SubscriptionToken Subscribe(Func&& callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto type = std::type_index(typeid(Event));
        auto& subscribers = subscribers_[type];
        const size_t id = ++nextId_;
        subscribers.push_back({
            id,
            [cb = std::forward<Func>(callback)](const void* eventPtr) {
                cb(*static_cast<const Event*>(eventPtr));
            }
        });
        return SubscriptionToken(type, id);
    }

    template<typename Event>
    void Publish(const Event& event) {
        std::vector<std::function<void(const void*)>> callbacks;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = subscribers_.find(std::type_index(typeid(Event)));
            if (it == subscribers_.end()) {
                return;
            }

            callbacks.reserve(it->second.size());
            for (const auto& subscriber : it->second) {
                callbacks.push_back(subscriber.callback);
            }
        }

        for (auto& callback : callbacks) {
            callback(&event);
        }
    }

    void Unsubscribe(const SubscriptionToken& token) {
        if (!token.IsValid()) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        auto it = subscribers_.find(token.type);
        if (it == subscribers_.end()) {
            return;
        }

        auto& list = it->second;
        for (auto iter = list.begin(); iter != list.end(); ++iter) {
            if (iter->id == token.id) {
                list.erase(iter);
                break;
            }
        }

        if (list.empty()) {
            subscribers_.erase(it);
        }
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        subscribers_.clear();
    }

private:
    struct Subscriber {
        size_t id;
        std::function<void(const void*)> callback;
    };

    std::unordered_map<std::type_index, std::vector<Subscriber>> subscribers_;
    size_t nextId_ = 0;
    std::mutex mutex_;
};

} // namespace ecs
