#include "../saves.hpp"

#include <Util.h>

#include <Geode/Geode.hpp>

#include <Geode/loader/ModSettingsManager.hpp>

using namespace geode::prelude;
using namespace cw::ferry;

namespace cw::ferry {
    namespace impl {
        static auto getAccountId() {
            return GJAccountManager::sharedState()->m_accountID;
        };

        static auto toBytes(std::string_view str) {
            return ByteVector{str.begin(), str.end()};
        };

        static auto fromBytes(ByteSpan data) {
            return std::string{data.begin(), data.end()};
        };
    };
};

#define CW_FERRY_ARGON_UNWRAP(var)                                                                  \
    auto tokenRes = co_await argon::startAuth();                                                    \
    if (tokenRes.isErr()) co_return WebRes(std::nullptr_t(), std::move(tokenRes).unwrapErr(), 402); \
    var = std::move(tokenRes).unwrap()

arc::Future<WebRes> save::uploadGameVars() {
    CW_FERRY_ARGON_UNWRAP(auto token);

    auto accountID = *co_await async::waitForMainThread<int>(impl::getAccountId);

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

    auto res = co_await request::setBytes(
        request::withAuth(accountID, std::move(token)),
        bw.writtenVec())
                   .post("/api/v1/upload"_api);

    co_return webres::processResp(res);
};

arc::Future<Result<StringMap<bool>>> save::downloadGameVars() {
    GEODE_CO_UNWRAP_INTO(auto token, co_await argon::startAuth());

    auto accountID = *co_await async::waitForMainThread<int>(impl::getAccountId);

    auto res = co_await request::withAuth(accountID, std::move(token))
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

    co_return Ok(std::move(vars));
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

arc::Future<WebRes> save::geode::uploadSettings() {
    CW_FERRY_ARGON_UNWRAP(auto token);

    auto accountID = *co_await async::waitForMainThread<int>(impl::getAccountId);

    auto const settings = save::geode::filterSettings(CW_GEODE_ID, save::geode::getSettings(CW_GEODE_ID));

    auto res = co_await request::setBytes(
        request::withAuth(accountID, std::move(token)),
        impl::toBytes(settings.dump(matjson::NO_INDENTATION)))
                   .post("/api/v1/upload-geode"_api);

    co_return webres::processResp(res);
};

arc::Future<Result<matjson::Value>> save::geode::downloadSettings() {
    GEODE_CO_UNWRAP_INTO(auto token, co_await argon::startAuth());

    auto accountID = *co_await async::waitForMainThread<int>(impl::getAccountId);

    auto res = co_await request::withAuth(accountID, std::move(token))
                   .get("/api/v1/download-geode"_api);

    if (res.error()) {
        auto const webResp = webres::processResp(res);
        co_return Err("{}: {}", webResp.getCode(), webResp.getError());
    };

    GEODE_CO_UNWRAP_INTO(auto json, matjson::Value::parse(impl::fromBytes(res.data())));
    co_return Ok(std::move(json));
};

void save::geode::applySettings(std::string_view modID, matjson::Value const& data) {
    return applySettings(Loader::get()->getInstalledMod(modID), data);
};

void save::geode::applySettings(Mod* mod, matjson::Value const& data) {
    auto const prev = mod->getSavedSettingsData();

    auto msm = ModSettingsManager::from(mod);

    (void)msm->load(data);
    auto const saved = msm->save();

    for (auto const& [key, value] : saved) {
        auto const& old = prev[key];

        if (value != old) queueInMainThread([mod, key]() {
            log::trace("Sending setting change event for {}/{}", mod->getID(), key);
            SettingChangedEvent(mod->getID(), key).send(mod->getSetting(key));
        });
    };
};

matjson::Value save::geode::filterSettings(std::string_view modID, matjson::Value const& data) {
    return filterSettings(Loader::get()->getInstalledMod(modID), data);
};

matjson::Value save::geode::filterSettings(Mod* mod, matjson::Value const& data) {
    matjson::Value out;

    for (auto const& [key, v] : data) {
        if (typeinfo_pointer_cast<FileSetting>(mod->getSetting(key))) continue;
        out[key] = v;
    };

    return out;
};

matjson::Value& save::geode::getSettings(std::string_view modID) {
    return getSettings(Loader::get()->getInstalledMod(modID));
};

matjson::Value& save::geode::getSettings(Mod* mod) {
    auto msm = ModSettingsManager::from(mod);

    msm->save();
    return msm->getSaveData();
};

arc::Future<WebRes> save::geode::mods::uploadSettings() {
    CW_FERRY_ARGON_UNWRAP(auto token);

    auto accountID = *co_await async::waitForMainThread<int>(impl::getAccountId);

    auto const settings = save::geode::mods::getAllSettings();

    auto res = co_await request::setBytes(
        request::withAuth(accountID, std::move(token)),
        impl::toBytes(settings.dump(matjson::NO_INDENTATION)))
                   .post("/api/v1/upload-geode-mods"_api);

    co_return webres::processResp(res);
};

arc::Future<::geode::Result<matjson::Value>> save::geode::mods::downloadSettings() {
    GEODE_CO_UNWRAP_INTO(auto token, co_await argon::startAuth());

    auto accountID = *co_await async::waitForMainThread<int>(impl::getAccountId);

    auto res = co_await request::withAuth(accountID, std::move(token))
                   .get("/api/v1/download-geode-mods"_api);

    if (res.error()) {
        auto const webResp = webres::processResp(res);
        co_return Err("{}: {}", webResp.getCode(), webResp.getError());
    };

    GEODE_CO_UNWRAP_INTO(auto json, matjson::Value::parse(impl::fromBytes(res.data())));
    co_return Ok(std::move(json));
};

matjson::Value save::geode::mods::getAllSettings() {
    matjson::Value out;

    auto const mods = Loader::get()->getAllMods();
    for (auto const& mod : mods) {
        if (mod->getID() != CW_GEODE_ID) out[mod->getID()] = filterSettings(mod, getSettings(mod));
    };

    return out;
};