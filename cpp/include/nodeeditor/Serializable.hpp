#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace nodeeditor {

class Serializable {
public:
    Serializable() : id_(reinterpret_cast<std::uintptr_t>(this)) {}

    virtual ~Serializable() = default;

    [[nodiscard]] std::uintptr_t objectId() const { return id_; }
    void setObjectId(std::uintptr_t value) { id_ = value; }

    [[nodiscard]] virtual std::string serialize() const {
        throw std::logic_error("serialize() not implemented");
    }

    virtual bool deserialize(const std::string& data,
                             std::unordered_map<std::uintptr_t, void*>& hashmap,
                             bool restoreId = true) {
        (void)data;
        (void)hashmap;
        (void)restoreId;
        throw std::logic_error("deserialize() not implemented");
    }

private:
    std::uintptr_t id_;
};

} // namespace nodeeditor
