#include "app_state.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <cstdlib>

AppState::AppState() {
    load();
}

// ── URL encode ────────────────────────────────────────────────────────────────
static std::string url_encode(const std::string& s) {
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

// ── Search ────────────────────────────────────────────────────────────────────
std::string AppState::search_url(const std::string& q) const {
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

// ── Persistence (settings + bookmarks only — no history ever) ─────────────────
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

    if (persist_bookmarks) {
        for (const auto& b : bookmarks)
            f << "bookmark=" << b.url << "\t" << b.title << "\n";
    }
}

void AppState::load() {
    std::ifstream f(config_path() + "/state.txt");
    if (!f) return;

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
        } else if (key == "bookmark") {
            auto tab = val.find('\t');
            if (tab != std::string::npos)
                bookmarks.push_back({val.substr(0, tab), val.substr(tab + 1)});
        }
    }
}
