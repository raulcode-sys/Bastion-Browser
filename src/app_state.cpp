#include "app_state.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <cstdlib>
#include <cctype>

AppState::AppState() {
    install_default_shortcuts();
    load();
}

// ── URL encode ────────────────────────────────────────────────────────────────
std::string AppState::url_encode(const std::string& s) {
    std::string out;
    char buf[4];
    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            out += c;
        else {
            std::snprintf(buf, sizeof(buf), "%%%02X", c);
            out += buf;
        }
    }
    return out;
}

// ── Default shortcuts installed on first run ──────────────────────────────────
void AppState::install_default_shortcuts() {
    shortcuts = {
        {"yt",  "YouTube",       "https://www.youtube.com/results?search_query=%s"},
        {"w",   "Wikipedia",     "https://en.wikipedia.org/wiki/Special:Search?search=%s"},
        {"gh",  "GitHub",        "https://github.com/search?q=%s"},
        {"so",  "Stack Overflow","https://stackoverflow.com/search?q=%s"},
        {"ddg", "DuckDuckGo",    "https://duckduckgo.com/?q=%s"},
        {"a",   "Archive.org",   "https://web.archive.org/web/*/%s"},
        {"r",   "Reddit",        "https://old.reddit.com/search?q=%s"},
        {"map", "OpenStreetMap", "https://www.openstreetmap.org/search?query=%s"},
    };
}

// ── Search ────────────────────────────────────────────────────────────────────
std::string AppState::search_url(const std::string& q) const {
    // Check for "keyword rest of query" form
    auto space = q.find(' ');
    if (space != std::string::npos) {
        std::string kw   = q.substr(0, space);
        std::string rest = q.substr(space + 1);
        for (const auto& sc : shortcuts) {
            if (sc.keyword == kw) {
                std::string url = sc.template_;
                auto pos = url.find("%s");
                if (pos != std::string::npos)
                    url.replace(pos, 2, url_encode(rest));
                return url;
            }
        }
    }

    std::string enc = url_encode(q);
    switch (search) {
        case SearchEngine::BRAVE:      return "https://search.brave.com/search?q=" + enc;
        case SearchEngine::DUCKDUCKGO: return "https://duckduckgo.com/?q=" + enc;
        case SearchEngine::STARTPAGE:  return "https://www.startpage.com/do/search?q=" + enc;
        case SearchEngine::SEARXNG:    return "https://searx.be/search?q=" + enc;
    }
    return "https://duckduckgo.com/?q=" + enc;
}

std::string AppState::theme_name() const {
    switch (theme) {
        case Theme::VISTA:         return "Retro Vista";
        case Theme::MODERN_DARK:   return "Modern Dark";
        case Theme::MODERN_LIGHT:  return "Modern Light";
    }
    return "Modern Dark";
}

std::string AppState::search_name() const {
    switch (search) {
        case SearchEngine::BRAVE:      return "Brave Search";
        case SearchEngine::DUCKDUCKGO: return "DuckDuckGo";
        case SearchEngine::STARTPAGE:  return "Startpage";
        case SearchEngine::SEARXNG:    return "SearXNG (searx.be)";
    }
    return "DuckDuckGo";
}

// ── Bookmarks ─────────────────────────────────────────────────────────────────
void AppState::add_bookmark(const std::string& url, const std::string& title) {
    if (is_bookmarked(url)) return;
    bookmarks.push_back({url, title});
    if (persist_bookmarks) save();
}

void AppState::remove_bookmark(const std::string& url) {
    bookmarks.erase(
        std::remove_if(bookmarks.begin(), bookmarks.end(),
            [&](const Bookmark& b){ return b.url == url; }),
        bookmarks.end());
    if (persist_bookmarks) save();
}

bool AppState::is_bookmarked(const std::string& url) const {
    for (const auto& b : bookmarks) if (b.url == url) return true;
    return false;
}

// ── Site settings ─────────────────────────────────────────────────────────────
std::string AppState::host_of(const std::string& url) {
    // http(s)://host[:port]/...
    auto scheme_end = url.find("://");
    if (scheme_end == std::string::npos) return "";
    auto host_start = scheme_end + 3;
    auto host_end = url.find_first_of("/:?#", host_start);
    std::string host = (host_end == std::string::npos)
        ? url.substr(host_start)
        : url.substr(host_start, host_end - host_start);
    // lowercase
    std::transform(host.begin(), host.end(), host.begin(),
        [](unsigned char c){ return std::tolower(c); });
    return host;
}

SiteSettings AppState::get_site(const std::string& host) const {
    auto it = site_rules.find(host);
    return (it != site_rules.end()) ? it->second : SiteSettings{};
}

void AppState::set_site(const std::string& host, const SiteSettings& s) {
    // If it's all-defaults, remove instead
    SiteSettings def;
    if (s.js_disabled == def.js_disabled &&
        s.force_dark  == def.force_dark  &&
        s.block_images == def.block_images)
    {
        site_rules.erase(host);
    } else {
        site_rules[host] = s;
    }
    save();
}

void AppState::clear_site(const std::string& host) {
    site_rules.erase(host);
    save();
}

// ── Session ───────────────────────────────────────────────────────────────────
void AppState::set_last_session(const std::vector<std::string>& urls) {
    last_session_tabs = urls;
    save();
}

// ── Persistence ───────────────────────────────────────────────────────────────
std::string AppState::config_path() const {
    const char* home = std::getenv("HOME");
    if (!home) home = "/tmp";
    return std::string(home) + "/.config/bastion";
}

void AppState::save() const {
    try { std::filesystem::create_directories(config_path()); }
    catch (...) { return; }

    std::ofstream f(config_path() + "/state.txt");
    if (!f) return;

    f << "theme="  << static_cast<int>(theme)  << "\n";
    f << "search=" << static_cast<int>(search) << "\n";
    f << "persist_bookmarks=" << (persist_bookmarks ? 1 : 0) << "\n";
    f << "new_tab_pos=" << static_cast<int>(new_tab_pos) << "\n";

    if (persist_bookmarks) {
        for (const auto& b : bookmarks)
            f << "bookmark=" << b.url << "\t" << b.title << "\n";
    }

    for (const auto& sc : shortcuts) {
        f << "shortcut=" << sc.keyword << "\t" << sc.name << "\t" << sc.template_ << "\n";
    }

    for (const auto& [host, s] : site_rules) {
        f << "site=" << host << "\t"
          << (s.js_disabled?1:0) << "\t"
          << (s.force_dark ?1:0) << "\t"
          << (s.block_images?1:0) << "\n";
    }

    for (const auto& url : last_session_tabs) {
        f << "session=" << url << "\n";
    }
}

void AppState::load() {
    std::ifstream f(config_path() + "/state.txt");
    if (!f) return;

    // If a saved state exists, wipe defaults first (so shortcuts don't dup)
    bool wiped_defaults = false;

    std::string line;
    while (std::getline(f, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        if (key == "theme") {
            try { theme = static_cast<Theme>(std::stoi(val)); } catch (...) {}
        } else if (key == "search") {
            try { search = static_cast<SearchEngine>(std::stoi(val)); } catch (...) {}
        } else if (key == "persist_bookmarks") {
            persist_bookmarks = (val == "1");
        } else if (key == "new_tab_pos") {
            try { new_tab_pos = static_cast<NewTabPos>(std::stoi(val)); } catch (...) {}
        } else if (key == "bookmark") {
            auto tab = val.find('\t');
            if (tab != std::string::npos)
                bookmarks.push_back({val.substr(0, tab), val.substr(tab + 1)});
        } else if (key == "shortcut") {
            if (!wiped_defaults) { shortcuts.clear(); wiped_defaults = true; }
            auto t1 = val.find('\t');
            if (t1 == std::string::npos) continue;
            auto t2 = val.find('\t', t1 + 1);
            if (t2 == std::string::npos) continue;
            shortcuts.push_back({
                val.substr(0, t1),
                val.substr(t1 + 1, t2 - t1 - 1),
                val.substr(t2 + 1)
            });
        } else if (key == "site") {
            auto t1 = val.find('\t'); if (t1 == std::string::npos) continue;
            auto t2 = val.find('\t', t1+1); if (t2 == std::string::npos) continue;
            auto t3 = val.find('\t', t2+1); if (t3 == std::string::npos) continue;
            std::string host = val.substr(0, t1);
            SiteSettings s;
            s.js_disabled  = (val[t1+1] == '1');
            s.force_dark   = (val[t2+1] == '1');
            s.block_images = (val[t3+1] == '1');
            site_rules[host] = s;
        } else if (key == "session") {
            last_session_tabs.push_back(val);
        }
    }
}
