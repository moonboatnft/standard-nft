#include <nft.hpp>
#include <json11.hpp>

using namespace json11;

void nft::createcol(const name& author, uint16_t royalty, const string& name, const string& image, const string& banner, const string& description, const std::map<string, string>& links) {
    require_auth(author);

    check(royalty >= 0, "royalty must be positive");
    check(royalty <= 1000, "royalty must be less than 1000");
    check(is_account(author), "author account does not exist");

    Json::object linksjson = Json::object();
    string socials[] = { "url", "twitter", "facebook", "discord", "instagram", "medium", "telegram" };
    for (auto i = links.begin(); i != links.end(); i++) {
        check(in_array(i->first, socials), "unsupported social media links");
        linksjson[i->first] = Json(i->second);
    }

    Json metadata = Json::object {
        { "name", name },
        { "image", image },
        { "banner", banner },
        { "description", description },
        { "links", linksjson },
    };

    string data = metadata.dump();
    check(data.size() <= 65535, "data has more than 65535 bytes");

    collections colstable(get_self(), get_self().value);

    uint64_t collection_id = new_id("collection"_n);
    colstable.emplace(author, [&](auto& s) {
        s.collection_id = collection_id;
        s.author        = author;
        s.royalty       = royalty;
        s.data          = data;
    });

    // collog
    auto logdata = std::make_tuple(collection_id, author, royalty, data);
    action(self_permission, _self, "collog"_n, logdata).send();
}

void nft::setroyalty(uint64_t collection_id, uint16_t royalty) {
    check(royalty >= 0, "royalty must be positive");
    check(royalty <= 1000, "royalty must be less than 1000");

    collections colstable(get_self(), get_self().value);
    auto col_it = colstable.require_find(collection_id, "unable to find collection");
    require_auth(col_it->author);

    colstable.modify(col_it, same_payer, [&](auto& s) {
        s.royalty       = royalty;
    });

    // collog
    auto logdata = std::make_tuple(collection_id, col_it->author, royalty, col_it->data);
    action(self_permission, _self, "collog"_n, logdata).send();
}

void nft::setlinks(uint64_t collection_id, const std::map<string, string>& links) {
    collections colstable(get_self(), get_self().value);
    auto col_it = colstable.require_find(collection_id, "unable to find collection");
    require_auth(col_it->author);

    string err;
    Json metadata = Json::parse(col_it->data, err);
    check(err == "", "parse data error: " + err);

    Json::object linksjson = Json::object();
    string socials[] = { "url", "twitter", "facebook", "discord", "instagram", "medium", "telegram" };
    for (auto i = links.begin(); i != links.end(); i++) {
        check(in_array(i->first, socials), "unsupported social media links");
        linksjson[i->first] = Json(i->second);
    }
    auto metajson = metadata.object_items();
    metajson["links"] = linksjson;

    string data = Json(metajson).dump();
    check(data.size() <= 65535, "data has more than 65535 bytes");

    colstable.modify(col_it, same_payer, [&](auto& s) {
        s.data       = data;
    });

    // collog
    auto logdata = std::make_tuple(collection_id, col_it->author, col_it->royalty, col_it->data);
    action(self_permission, _self, "collog"_n, logdata).send();
}

void nft::createasset(uint64_t collection_id, uint64_t supply, uint64_t max_supply, const string& name, const string& image, const string& animation_url, const string& external_url, const string& description, const std::map<string, string> attributes) {
    check(max_supply > 0, "max-supply must be positive");
    
    Json::object metadata = Json::object {
        { "name", name },
        { "image", image },
        { "description", description },
    };

    if (animation_url != "") {
        metadata["animation_url"] = animation_url;
    }
    if (external_url != "") {
        metadata["external_url"] = external_url;
    }

    Json::array jAttrs = Json::array();
    for (auto i = attributes.begin(); i != attributes.end(); i++) {
        jAttrs.push_back(Json::object {
            { "trait_type", i->first },
            { "value", i->second },
        });
    }
    metadata["attributes"] = jAttrs;

    string data = Json(metadata).dump();
    check(data.size() <= 65535, "data has more than 65535 bytes");

    collections colstable(get_self(), get_self().value);
    auto col_it = colstable.require_find(collection_id, "unable to find collection");
    require_auth(col_it->author);

    assets assetstable(get_self(), get_self().value);

    uint64_t asset_id = new_id("asset"_n);
    assetstable.emplace(col_it->author, [&](auto& s) {
        s.asset_id      = asset_id;
        s.collection_id = collection_id;
        s.supply        = 0;
        s.max_supply    = max_supply;
        s.data          = data;
    });

    // assetlog
    auto logdata = std::make_tuple(asset_id, collection_id, max_supply, data);
    action(self_permission, _self, "assetlog"_n, logdata).send();

    if (supply > 0) {
        mint(col_it->author, asset_id, supply, string("create and mint"));
    }
}

void nft::mint(const name& to, uint64_t asset_id, int64_t amount, const string& memo) {
    check(memo.size() <= 256, "memo has more than 256 bytes");

    assets assetstable(get_self(), get_self().value);
    auto ast_it = assetstable.require_find(asset_id, "unable to find asset");

    collections colstable(get_self(), get_self().value);
    auto col_it = colstable.require_find(ast_it->collection_id, "unable to find collection");
    require_auth(col_it->author);

    check(amount > 0, "must issue positive amount");
    check(ast_it->max_supply >= ast_it->supply + amount, "amount exceeds available supply");

    assetstable.modify(ast_it, same_payer, [&](auto& s) {
        s.supply += amount;
    });

     auto to_balance = add_balance(to, asset_id, amount, col_it->author);

    // transfer log
    auto data = std::make_tuple(name(""), col_it->author, asset_id, amount, int64_t(0), to_balance, memo);
    action(self_permission, _self, "transferlog"_n, data).send();

    // transfer
    if (to != col_it->author) {
        auto data = std::make_tuple(col_it->author, to, asset_id, amount, memo);
        action(permission_level{col_it->author, "active"_n}, _self, "transfer"_n, data).send();
    }
}

void nft::burn(uint64_t asset_id, int64_t amount, const string& memo) {
    check(memo.size() <= 256, "memo has more than 256 bytes");
    check(amount > 0, "must retire positive amount");

    assets assetstable(get_self(), get_self().value);
    auto ast_it = assetstable.require_find(asset_id, "unable to find asset");
    check(ast_it->supply >= amount, "insufficient amount");

    collections colstable(get_self(), get_self().value);
    auto col_it = colstable.require_find(ast_it->collection_id, "unable to find collection");
    require_auth(col_it->author);

    assetstable.modify(ast_it, same_payer, [&](auto& s) {
       s.supply -= amount;
    });

    auto from_balance = sub_balance(col_it->author, asset_id, amount);

    // transfer log
    auto data = std::make_tuple(col_it->author, name(""), asset_id, amount, from_balance, int64_t(0), memo);
    action(self_permission, _self, "transferlog"_n, data).send();
}

void nft::transfer(const name& from, const name& to, uint64_t asset_id, int64_t amount, const string& memo) {
    check(from != to, "cannot transfer to self");
    require_auth(from);
    check(is_account(to), "to account does not exist");

    assets assetstable(get_self(), get_self().value);
    const auto& ast = assetstable.get(asset_id, "unable to find asset");

    require_recipient(from);
    require_recipient(to);

    check(amount > 0, "must transfer positive amount");
    check(memo.size() <= 256, "memo has more than 256 bytes");

    auto payer = has_auth(to) ? to : from;

    auto from_balance = sub_balance(from, asset_id, amount);
    auto to_balance = add_balance(to, asset_id, amount, payer);

    // transfer log
    auto data = std::make_tuple(from, to, asset_id, amount, from_balance, to_balance, memo);
    action(self_permission, _self, "transferlog"_n, data).send();
}

int64_t nft::sub_balance(const name& owner, uint64_t asset_id, int64_t amount) {
    balances from_blns(get_self(), owner.value);

    const auto& from = from_blns.get(asset_id, "no balance object found");
    check(from.balance >= amount, "overdrawn balance");

    if (from.balance == amount) {
        from_blns.erase(from);
        return 0;
    } else {
        from_blns.modify(from, owner, [&](auto& a) {
            a.balance -= amount;
        });
    }
    return from.balance;
}

int64_t nft::add_balance(const name& owner, uint64_t asset_id, int64_t amount, const name& ram_payer) {
    balances to_blns(get_self(), owner.value);
    auto to = to_blns.find(asset_id);
    if (to == to_blns.end()) {
        to = to_blns.emplace(ram_payer, [&](auto& a){
            a.asset_id = asset_id;
            a.balance = amount;
        });
    } else {
        to_blns.modify(to, same_payer, [&](auto& a) {
            a.balance += amount;
        });
    }
    return to->balance;
}