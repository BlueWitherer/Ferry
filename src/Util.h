#pragma once

#include <argon/argon.hpp>

#include <dbuf/ByteReader.hpp>
#include <dbuf/ByteWriter.hpp>

#include <ui/Include.h>
#include <util/Include.h>

#include <Geode/Geode.hpp>

namespace cw::ferry {
    namespace str = geode::utils::string;

    namespace request {
        inline auto base() {
            auto loader = geode::Loader::get();

            return geode::utils::web::WebRequest()
                .userAgent(fmt::format("Ferry/{} ({}, Geode {}, GD {})",
                    geode::Mod::get()->getVersion().toVString(false),
                    geode::utils::platform::getString(),
                    loader->getVersion(),
                    loader->getGameVersion()))
                .timeout(std::chrono::seconds(15));
        };
    };
};