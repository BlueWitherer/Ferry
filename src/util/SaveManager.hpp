#pragma once

#include "base/Singleton.hpp"

#include "WebRes.hpp"

#include <Geode/Geode.hpp>

namespace cw::ferry {
    class SaveManager final : public base::Singleton<SaveManager> {
    public:
        arc::Future<WebRes> uploadGameVars();
        arc::Future<geode::Result<geode::utils::StringMap<bool>>> downloadGameVars();
    };

    struct ProgressEvent final : geode::Event<ProgressEvent, bool(float)> {
        using Event::Event;
    };
};