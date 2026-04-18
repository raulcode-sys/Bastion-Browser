#pragma once
#include <string>
#include <vector>

enum class Theme        { VISTA, MODERN_DARK, MODERN_LIGHT };
enum class SearchEngine { BRAVE, DUCKDUCKGO, STARTPAGE, SEARXNG };

struct Bookmark {
    std::string url;
    std::string title;
};

// ─────────────────────────────────────────────────────────────────────────────
//  AppState – in-memory browser state.
//  NO history is ever recorded — true privacy.
//  Bookmarks are opt-in persistent.
// ─────────────────────────────────────────────────────────────────────────────
class AppState {
public:
    AppState();

    Theme         theme        = Theme::MODERN_DARK;
    SearchEngine  search       = SearchEngine::DUCKDUCKGO;
    bool          persist_bookmarks = true;

    std::vector<Bookmark> bookmarks;

    // Session-only counters (never written to disk)
    int trackers_blocked_session = 0;
    int pages_visited_session    = 0;

    void add_bookmark   (const std::string& url, const std::string& title);
    void remove_bookmark(const std::string& url);
    bool is_bookmarked  (const std::string& url) const;

    std::string search_url  (const std::string& query) const;
    std::string theme_name  () const;
    std::string search_name () const;

    void load();
    void save() const;

private:
    std::string config_path() const;
};
