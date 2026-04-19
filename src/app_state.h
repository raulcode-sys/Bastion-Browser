#pragma once
#include <string>
#include <vector>
#include <map>

enum class Theme        { VISTA, MODERN_DARK, MODERN_LIGHT };
enum class SearchEngine { BRAVE, DUCKDUCKGO, STARTPAGE, SEARXNG };
enum class NewTabPos    { AFTER_CURRENT, AT_END };

struct Bookmark {
    std::string url;
    std::string title;
};

// Per-domain overrides. Default empty map = no overrides, everything default.
struct SiteSettings {
    bool js_disabled   = false;
    bool force_dark    = false;
    bool block_images  = false;
};

// A search shortcut: typing "yt cats" expands to the URL template with "cats"
// substituted for %s.
struct SearchShortcut {
    std::string keyword;   // e.g. "yt"
    std::string name;      // e.g. "YouTube"
    std::string template_; // e.g. "https://youtube.com/results?search_query=%s"
};

// ─────────────────────────────────────────────────────────────────────────────
//  AppState – settings, bookmarks, search shortcuts, per-site rules.
//  No browsing history is ever recorded.
// ─────────────────────────────────────────────────────────────────────────────
class AppState {
public:
    AppState();

    // ── Settings ─────────────────────────────────────────────────────────────
    Theme         theme             = Theme::MODERN_DARK;
    SearchEngine  search            = SearchEngine::DUCKDUCKGO;
    bool          persist_bookmarks = true;
    NewTabPos     new_tab_pos       = NewTabPos::AT_END;

    // Fortress mode: session-only, not persisted. Toggled live.
    bool          fortress_mode     = false;

    // ── Data ─────────────────────────────────────────────────────────────────
    std::vector<Bookmark>                 bookmarks;
    std::vector<SearchShortcut>           shortcuts;
    std::map<std::string, SiteSettings>   site_rules;   // host -> rules

    // Saved tabs from last session (for manual restore). Written on clean exit.
    std::vector<std::string>              last_session_tabs;

    // Session counters
    int trackers_blocked_session = 0;

    // ── Bookmarks ────────────────────────────────────────────────────────────
    void add_bookmark   (const std::string& url, const std::string& title);
    void remove_bookmark(const std::string& url);
    bool is_bookmarked  (const std::string& url) const;

    // ── Search ───────────────────────────────────────────────────────────────
    // Expands "yt cats" via shortcuts; falls back to default search engine.
    std::string search_url(const std::string& query) const;

    // ── Site settings ────────────────────────────────────────────────────────
    SiteSettings get_site(const std::string& host) const;
    void set_site(const std::string& host, const SiteSettings& s);
    void clear_site(const std::string& host);
    static std::string host_of(const std::string& url);

    // ── Session ──────────────────────────────────────────────────────────────
    void set_last_session(const std::vector<std::string>& urls);

    // ── Names for UI ─────────────────────────────────────────────────────────
    std::string theme_name () const;
    std::string search_name() const;

    // ── Persistence ──────────────────────────────────────────────────────────
    void load();
    void save() const;

private:
    std::string config_path() const;
    static std::string url_encode(const std::string& s);
    void install_default_shortcuts();
};
