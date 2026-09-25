#pragma once

#include "base/Singleton.hpp"

#include "WebRes.hpp"

#include <Geode/Geode.hpp>

namespace cw::ferry {
    namespace save {
        arc::Future<WebRes> uploadGameVars();
        arc::Future<geode::Result<geode::utils::StringMap<bool>>> downloadGameVars();

        void applyGameVars(geode::utils::StringMap<bool> const& vars);
    };
};