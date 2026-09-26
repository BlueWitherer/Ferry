#include "../SaveManager.hpp"

#include <Util.h>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace cw::ferry;

arc::Future<WebRes> save::uploadGameVars() {
    auto tokenRes = co_await argon::startAuth();
    if (tokenRes.isErr()) co_return WebRes(std::nullptr_t(), std::move(tokenRes).unwrapErr(), 402);

    auto token = std::move(tokenRes).unwrap();

    auto const accountID = *co_await async::waitForMainThread<int>([]() {
        return GJAccountManager::sharedState()->m_accountID;
    });

    auto vars = *co_await async::waitForMainThread<StringMap<bool>>([]() {
        StringMap<bool> vars;

        auto gm = GameManager::sharedState();
        auto dict = gm->m_valueKeeper->asExt();

        log::trace("Iterating through {} variables...", dict.size());

        for (auto const& [key, v] : dict) {
            if (str::startsWith(key, "gv_0")) {
                auto const k = str::filter(key, "0123456789");
                log::trace("Parsing game variable {}...", k);
                vars[k] = gm->getGameVariable(k.c_str());
            };
        };

        log::debug("Parsed {} variables", vars.size());
        return vars;
    });

    dbuf::ByteWriter bw;

    bw.writeU64(vars.size());

    for (auto const& [str, value] : vars) {
        bw.writeStringU8(str);
        bw.writeBool(value);
    };

    auto res = co_await request::base()
                   .body(bw.writtenVec())
                   .param("account_id", accountID)
                   .param("authtoken", std::move(token))
                   .post("/api/v1/upload"_api);

    co_return webres::processResp(res);
};

arc::Future<Result<StringMap<bool>>> save::downloadGameVars() {
    GEODE_CO_UNWRAP_INTO(std::string token, co_await argon::startAuth());

    auto const accountID = *co_await async::waitForMainThread<int>([]() {
        return GJAccountManager::sharedState()->m_accountID;
    });

    auto res = co_await request::base()
                   .param("account_id", accountID)
                   .param("authtoken", std::move(token))
                   .get("/api/v1/download"_api);

    if (res.error()) {
        auto const webResp = webres::processResp(res);
        co_return Err("{}: {}", webResp.getCode(), webResp.getError());
    };

    dbuf::ByteReader br{res.data()};

    GEODE_CO_UNWRAP_INTO(size_t size, br.readU64());

    log::debug("Received {} game variables", size);
    if (size <= 0) co_return Err("Map stream has invalid size");

    StringMap<bool> vars;
    vars.reserve(size);

    for (size_t i = 0; i < size; ++i) {
        GEODE_CO_UNWRAP_INTO(std::string key, br.readStringU8());
        GEODE_CO_UNWRAP_INTO(bool val, br.readBool());

        vars[std::move(key)] = val;
    };

    co_return Ok(vars);
};

void save::applyGameVars(StringMap<bool> const& vars) {
    auto gm = GameManager::sharedState();
    for (auto const& [key, val] : vars) {
        if (key.size() != 4) {
            log::error("Key {} is not valid", key);
            continue;
        };

        gm->setGameVariable(key.c_str(), val);
    };
};