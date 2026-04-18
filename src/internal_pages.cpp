#include "internal_pages.h"
#include "download_manager.h"
#include <sstream>
#include <iomanip>

namespace internal_pages {

// ── HTML-escape helper ─────────────────────────────────────────────────────
static std::string esc(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&#39;";  break;
            default:   out += c;
        }
    }
    return out;
}

// ── Resolve theme colors (used by internal pages) ───────────────────────────
struct Palette {
    const char* bg;
    const char* bg2;
    const char* fg;
    const char* dim;
    const char* accent;
    const char* border;
    const char* success;
    const char* danger;
    const char* font;
};

static Palette palette_for(Theme t) {
    switch (t) {
        case Theme::MODERN_DARK:
            return {"#0f0f14","#1c1c24","#e8e8ea","#808088","#7aa2ff","#2a2a36","#5cffa0","#ff6b6b",
                    "\"Inter\",\"Segoe UI\",sans-serif"};
        case Theme::MODERN_LIGHT:
            return {"#fafafa","#ffffff","#1a1a1f","#6a6a72","#3d6bd3","#dcdce0","#1a8f3a","#c62828",
                    "\"Inter\",\"Segoe UI\",sans-serif"};
        case Theme::VISTA:
            return {"#dce9f5","#ffffff","#0a2a50","#3a5070","#1a5fa8","#6a9fd8","#005a00","#aa0000",
                    "\"Segoe UI\",\"Tahoma\",sans-serif"};
    }
    return {"#0f0f14","#1c1c24","#e8e8ea","#808088","#7aa2ff","#2a2a36","#5cffa0","#ff6b6b",
            "\"Inter\",sans-serif"};
}

// Vista-specific page background (aero gradient)
static std::string page_bg(Theme t) {
    if (t == Theme::VISTA)
        return "linear-gradient(180deg,#dce9f5 0%,#c2d8ee 40%,#b8d0e8 100%)";
    Palette p = palette_for(t);
    return p.bg;
}

// Shared CSS for all internal pages
static std::string base_css(const AppState& s) {
    Palette p = palette_for(s.theme);
    std::ostringstream o;
    o << "<style>"
      << "*{box-sizing:border-box;margin:0;padding:0}"
      << "body{background:" << page_bg(s.theme) << ";color:" << p.fg
          << ";font-family:" << p.font << ";min-height:100vh;padding:40px 20px}"
      << ".wrap{max-width:900px;margin:0 auto}"
      << "h1{font-size:28px;margin-bottom:8px;font-weight:600}"
      << "h2{font-size:16px;margin:24px 0 12px;color:" << p.dim
          << ";text-transform:uppercase;letter-spacing:1px;font-weight:600}"
      << "a{color:" << p.accent << ";text-decoration:none}"
      << "a:hover{text-decoration:underline}"
      << ".card{background:" << p.bg2 << ";border:1px solid " << p.border
          << ";border-radius:12px;padding:20px;margin-bottom:16px}"
      << ".row{display:flex;justify-content:space-between;align-items:center;"
         "padding:12px 0;border-bottom:1px solid " << p.border << "}"
      << ".row:last-child{border-bottom:none}"
      << "button,.btn{background:" << p.accent << ";color:#fff;border:none;"
         "border-radius:8px;padding:8px 16px;font-size:13px;font-weight:500;"
         "cursor:pointer;font-family:inherit;text-decoration:none;display:inline-block}"
      << "button:hover,.btn:hover{opacity:0.85;text-decoration:none}"
      << ".btn-danger{background:" << p.danger << "}"
      << ".btn-ghost{background:transparent;color:" << p.fg
          << ";border:1px solid " << p.border << "}"
      << ".dim{color:" << p.dim << ";font-size:13px}"
      << ".nav{display:flex;gap:16px;margin-bottom:32px;padding-bottom:16px;"
         "border-bottom:1px solid " << p.border << "}"
      << ".nav a{color:" << p.dim << ";font-weight:500;font-size:13px}"
      << ".nav a.active{color:" << p.accent << "}"
      << "input[type=text],select{background:" << p.bg << ";color:" << p.fg
          << ";border:1px solid " << p.border << ";border-radius:8px;"
         "padding:8px 12px;font-family:inherit;font-size:13px;width:100%}"
      << "input[type=text]:focus,select:focus{outline:none;border-color:" << p.accent << "}"
      << "</style>";
    return o.str();
}

// Shared top nav bar
static std::string nav_bar(const std::string& active) {
    auto link = [&](const std::string& slug, const std::string& label) {
        std::string cls = (active == slug) ? "active" : "";
        return "<a href=\"bastion://" + slug + "\" class=\"" + cls + "\">" + label + "</a>";
    };
    return "<div class=\"nav\">"
         + link("home",      "Home")
         + link("bookmarks", "Bookmarks")
         + link("downloads", "Downloads")
         + link("settings",  "Settings")
         + "</div>";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Home page
// ─────────────────────────────────────────────────────────────────────────────
std::string home(const AppState& s) {
    Palette p = palette_for(s.theme);
    std::ostringstream o;
    o << "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Bastion</title>"
      << base_css(s)
      << "<style>"
      << ".hero{text-align:center;padding:60px 20px 40px}"
      << ".logo{font-size:64px;font-weight:700;letter-spacing:-2px;"
         "background:linear-gradient(135deg," << p.accent << "," << p.success
         << ");-webkit-background-clip:text;-webkit-text-fill-color:transparent;"
         "background-clip:text;margin-bottom:12px}"
      << ".tagline{color:" << p.dim << ";font-size:14px;margin-bottom:40px}"
      << ".search{max-width:600px;margin:0 auto 40px}"
      << ".search form{display:flex;gap:8px}"
      << ".search input{flex:1;padding:14px 20px;font-size:15px;border-radius:24px;"
         "background:" << p.bg2 << ";border:2px solid " << p.border << "}"
      << ".search input:focus{border-color:" << p.accent << "}"
      << ".search button{border-radius:24px;padding:0 24px;font-size:14px}"
      << ".stats{display:grid;grid-template-columns:repeat(3,1fr);gap:12px;"
         "max-width:600px;margin:0 auto 40px}"
      << ".stat{background:" << p.bg2 << ";border:1px solid " << p.border
          << ";border-radius:12px;padding:16px;text-align:center}"
      << ".stat-val{font-size:28px;font-weight:700;color:" << p.accent << "}"
      << ".stat-lbl{font-size:11px;color:" << p.dim
          << ";text-transform:uppercase;letter-spacing:1px;margin-top:4px}"
      << ".quick{max-width:600px;margin:0 auto}"
      << ".quick-grid{display:grid;grid-template-columns:repeat(4,1fr);gap:8px}"
      << ".quick a{display:block;padding:16px 8px;background:" << p.bg2
          << ";border:1px solid " << p.border << ";border-radius:12px;"
         "text-align:center;color:" << p.fg << ";font-size:12px;"
         "overflow:hidden;text-overflow:ellipsis;white-space:nowrap}"
      << ".quick a:hover{border-color:" << p.accent << ";text-decoration:none}"
      << "</style></head><body><div class=\"wrap\">"
      << "<div class=\"hero\">"
      << "<div class=\"logo\">Bastion</div>"
      << "<div class=\"tagline\">Private · Ephemeral · Hardened</div>"
      << "<div class=\"search\"><form method=\"get\" action=\"bastion://search\">"
      << "<input type=\"text\" name=\"q\" placeholder=\"Search " << esc(s.search_name())
      << " or enter a URL\" autofocus>"
      << "<button type=\"submit\">Search</button></form></div>"
      << "<div class=\"stats\">"
      << "<div class=\"stat\"><div class=\"stat-val\">" << s.trackers_blocked_session
      << "</div><div class=\"stat-lbl\">Trackers Blocked</div></div>"
      << "<div class=\"stat\"><div class=\"stat-val\">∞</div>"
         "<div class=\"stat-lbl\">No History Kept</div></div>"
      << "<div class=\"stat\"><div class=\"stat-val\">" << s.bookmarks.size()
      << "</div><div class=\"stat-lbl\">Bookmarks</div></div>"
      << "</div></div>";

    // Quick access: top 8 bookmarks
    if (!s.bookmarks.empty()) {
        o << "<div class=\"quick\"><h2>Quick Access</h2><div class=\"quick-grid\">";
        size_t shown = 0;
        for (const auto& b : s.bookmarks) {
            if (shown++ >= 8) break;
            o << "<a href=\"" << esc(b.url) << "\" title=\"" << esc(b.url) << "\">"
              << esc(b.title.empty() ? b.url : b.title) << "</a>";
        }
        o << "</div></div>";
    }

    o << "<div style=\"text-align:center;margin-top:40px\">"
      << nav_bar("home")
      << "</div>";
    o << "</div></body></html>";
    return o.str();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Settings page
// ─────────────────────────────────────────────────────────────────────────────
static std::string theme_row(const AppState& s, Theme t, const std::string& label) {
    std::ostringstream o;
    int ti = static_cast<int>(t);
    bool active = (s.theme == t);
    o << "<div class=\"row\">"
      << "<div><strong>" << label << "</strong>"
      << (active ? " <span style=\"color:#5cffa0\">✓ active</span>" : "")
      << "</div>"
      << "<a class=\"btn " << (active ? "btn-ghost" : "") << "\" "
      << "href=\"bastion://act?a=theme&v=" << ti << "\">"
      << (active ? "In use" : "Use") << "</a></div>";
    return o.str();
}

static std::string search_row(const AppState& s, SearchEngine eng, const std::string& label) {
    std::ostringstream o;
    int si = static_cast<int>(eng);
    bool active = (s.search == eng);
    o << "<div class=\"row\">"
      << "<div><strong>" << label << "</strong></div>"
      << "<a class=\"btn " << (active ? "btn-ghost" : "") << "\" "
      << "href=\"bastion://act?a=search&v=" << si << "\">"
      << (active ? "In use" : "Use") << "</a></div>";
    return o.str();
}

std::string settings(const AppState& s) {
    std::ostringstream o;
    o << "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Settings — Bastion</title>"
      << base_css(s) << "</head><body><div class=\"wrap\">"
      << nav_bar("settings")
      << "<h1>Settings</h1>"
      << "<p class=\"dim\">All changes take effect immediately.</p>";

    // Appearance
    o << "<h2>Appearance</h2><div class=\"card\">"
      << theme_row(s, Theme::MODERN_DARK,  "Modern Dark")
      << theme_row(s, Theme::MODERN_LIGHT, "Modern Light")
      << theme_row(s, Theme::VISTA,        "Retro Windows Vista Aero")
      << "</div>";

    // Search engine
    o << "<h2>Search Engine</h2><div class=\"card\">"
      << search_row(s, SearchEngine::DUCKDUCKGO, "DuckDuckGo")
      << search_row(s, SearchEngine::BRAVE,      "Brave Search")
      << search_row(s, SearchEngine::STARTPAGE,  "Startpage")
      << search_row(s, SearchEngine::SEARXNG,    "SearXNG (searx.be)")
      << "</div>";

    // Privacy & Data
    o << "<h2>Privacy &amp; Data</h2><div class=\"card\">"
      << "<div class=\"row\"><div><strong>Browsing history</strong>"
         "<div class=\"dim\">Bastion never records browsing history. Nothing to clear.</div></div>"
         "<div class=\"dim\" style=\"color:#5cffa0\">✓ Never stored</div></div>"
      << "<div class=\"row\"><div><strong>Persist bookmarks</strong>"
         "<div class=\"dim\">Saved to ~/.config/bastion/state.txt. Turn off for session-only.</div></div>"
      << "<a class=\"btn\" href=\"bastion://act?a=toggle_bookmarks\">"
      << (s.persist_bookmarks ? "On — click to disable" : "Off — click to enable")
      << "</a></div>"
      << "<div class=\"row\"><div><strong>Clear all bookmarks</strong>"
         "<div class=\"dim\">" << s.bookmarks.size() << " bookmarks</div></div>"
      << "<a class=\"btn btn-danger\" href=\"bastion://act?a=clear_bookmarks\">Clear</a></div>"
      << "<div class=\"row\"><div><strong>Cookies &amp; cache</strong>"
         "<div class=\"dim\">Bastion uses an ephemeral session — nothing is written to disk.</div></div>"
         "<div class=\"dim\" style=\"color:#5cffa0\">✓ Ephemeral</div></div>"
      << "</div>";

    // About
    o << "<h2>About</h2><div class=\"card\">"
      << "<div class=\"row\"><div><strong>Bastion Browser</strong>"
         "<div class=\"dim\">Version 1.0.0 · Built on WebKitGTK</div></div></div>"
      << "<div class=\"row\"><div><strong>Current theme</strong></div>"
         "<div class=\"dim\">" << esc(s.theme_name()) << "</div></div>"
      << "<div class=\"row\"><div><strong>Current search</strong></div>"
         "<div class=\"dim\">" << esc(s.search_name()) << "</div></div>"
      << "<div class=\"row\"><div><strong>Trackers blocked (session)</strong></div>"
         "<div class=\"dim\">" << s.trackers_blocked_session << "</div></div>"
      << "</div>";

    o << "</div></body></html>";
    return o.str();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Bookmarks page
// ─────────────────────────────────────────────────────────────────────────────
std::string bookmarks(const AppState& s) {
    std::ostringstream o;
    o << "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Bookmarks — Bastion</title>"
      << base_css(s) << "</head><body><div class=\"wrap\">"
      << nav_bar("bookmarks")
      << "<h1>Bookmarks</h1>"
      << "<p class=\"dim\">" << s.bookmarks.size() << " saved. "
      << (s.persist_bookmarks ? "Persisted to disk." : "Session-only.") << "</p>";

    if (s.bookmarks.empty()) {
        o << "<div class=\"card\" style=\"text-align:center;padding:40px\">"
          << "<p class=\"dim\">No bookmarks yet. Click the ⭐ button in the toolbar "
             "while viewing a page to save it.</p></div>";
    } else {
        o << "<div class=\"card\">";
        for (const auto& b : s.bookmarks) {
            std::string u = b.url;
            // url-safe version for action link (simple — assumes normal URLs)
            o << "<div class=\"row\">"
              << "<div style=\"overflow:hidden;min-width:0\">"
              << "<a href=\"" << esc(u) << "\"><strong>"
              << esc(b.title.empty() ? u : b.title) << "</strong></a>"
              << "<div class=\"dim\" style=\"overflow:hidden;text-overflow:ellipsis;"
                 "white-space:nowrap\">" << esc(u) << "</div>"
              << "</div>"
              << "<a class=\"btn btn-danger\" "
                 "href=\"bastion://act?a=rm_bookmark&u=" << esc(u) << "\" "
                 "style=\"margin-left:16px;white-space:nowrap\">Remove</a>"
              << "</div>";
        }
        o << "</div>";
    }
    o << "</div></body></html>";
    return o.str();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Downloads page
// ─────────────────────────────────────────────────────────────────────────────
static std::string fmt_bytes(guint64 b) {
    if (b == 0) return "—";
    const char* units[] = {"B","KB","MB","GB","TB"};
    double v = static_cast<double>(b);
    int u = 0;
    while (v >= 1024.0 && u < 4) { v /= 1024.0; u++; }
    std::ostringstream o;
    o << std::fixed << std::setprecision(v < 10 ? 1 : 0) << v << " " << units[u];
    return o.str();
}

static std::string status_label(DLStatus s) {
    switch (s) {
        case DLStatus::STARTING:    return "Starting";
        case DLStatus::IN_PROGRESS: return "Downloading";
        case DLStatus::FINISHED:    return "Completed";
        case DLStatus::FAILED:      return "Failed";
        case DLStatus::CANCELLED:   return "Cancelled";
    }
    return "";
}

static std::string status_color(DLStatus s, const Palette& p) {
    switch (s) {
        case DLStatus::FINISHED:    return p.success;
        case DLStatus::FAILED:
        case DLStatus::CANCELLED:   return p.danger;
        default:                    return p.accent;
    }
}

std::string downloads(const AppState& s, const DownloadManager& dm) {
    Palette p = palette_for(s.theme);
    std::ostringstream o;
    o << "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
      << "<title>Downloads — Bastion</title>"
      << "<meta http-equiv=\"refresh\" content=\"2\">" // auto-refresh every 2s
      << base_css(s) << "</head><body><div class=\"wrap\">"
      << nav_bar("downloads")
      << "<h1>Downloads</h1>";

    const auto& items = dm.items();
    int active = dm.active_count();

    o << "<p class=\"dim\">";
    if (items.empty())       o << "No downloads yet.";
    else if (active > 0)     o << active << " active · " << items.size() << " total";
    else                     o << items.size() << " total";
    o << " · Saved to <code>~/Downloads/</code>";
    o << "</p>";

    if (!items.empty()) {
        o << "<div style=\"margin-bottom:16px\">"
          << "<a class=\"btn btn-ghost\" href=\"bastion://act?a=clear_downloads\">Clear completed</a>"
          << "</div>";

        o << "<div class=\"card\">";
        // newest first
        for (auto it = items.rbegin(); it != items.rend(); ++it) {
            const auto& d = **it;
            std::string scol = status_color(d.status, p);

            o << "<div class=\"row\" style=\"flex-direction:column;align-items:stretch\">"
              << "<div style=\"display:flex;justify-content:space-between;align-items:flex-start;gap:16px\">"
              << "<div style=\"overflow:hidden;min-width:0;flex:1\">"
              << "<strong>" << esc(d.filename.empty() ? d.url : d.filename) << "</strong>"
              << "<div class=\"dim\" style=\"overflow:hidden;text-overflow:ellipsis;"
                 "white-space:nowrap;font-size:11px\">" << esc(d.url) << "</div>"
              << "</div>"
              << "<div style=\"text-align:right;white-space:nowrap\">"
              << "<div style=\"color:" << scol << ";font-weight:600;font-size:12px\">"
              << status_label(d.status) << "</div>"
              << "<div class=\"dim\" style=\"font-size:11px\">";

            if (d.status == DLStatus::IN_PROGRESS || d.status == DLStatus::STARTING) {
                o << fmt_bytes(d.received_bytes);
                if (d.total_bytes > 0) o << " / " << fmt_bytes(d.total_bytes);
            } else {
                o << fmt_bytes(d.received_bytes);
            }
            o << "</div></div></div>";

            // Progress bar for active downloads
            if (d.status == DLStatus::IN_PROGRESS || d.status == DLStatus::STARTING) {
                int pct = static_cast<int>(d.progress * 100);
                o << "<div style=\"margin-top:8px;height:4px;background:" << p.border
                  << ";border-radius:2px;overflow:hidden\">"
                  << "<div style=\"height:100%;width:" << pct << "%;background:" << p.accent
                  << ";transition:width 0.2s\"></div></div>";
            }

            // Error message
            if (d.status == DLStatus::FAILED && !d.error_message.empty()) {
                o << "<div class=\"dim\" style=\"margin-top:6px;font-size:11px;color:"
                  << p.danger << "\">" << esc(d.error_message) << "</div>";
            }

            // Action buttons
            o << "<div style=\"margin-top:8px;display:flex;gap:8px\">";
            if (d.status == DLStatus::IN_PROGRESS || d.status == DLStatus::STARTING) {
                o << "<a class=\"btn btn-danger\" "
                     "href=\"bastion://act?a=cancel_dl&id=" << esc(d.id) << "\">Cancel</a>";
            }
            if (d.status == DLStatus::FINISHED) {
                o << "<a class=\"btn\" href=\"file://" << esc(d.destination) << "\">Open</a>";
                // Open containing folder
                std::string folder = d.destination;
                auto slash = folder.find_last_of('/');
                if (slash != std::string::npos) folder = folder.substr(0, slash);
                o << "<a class=\"btn btn-ghost\" href=\"file://" << esc(folder)
                  << "\">Show folder</a>";
            }
            if (d.status != DLStatus::IN_PROGRESS && d.status != DLStatus::STARTING) {
                o << "<a class=\"btn btn-ghost\" "
                     "href=\"bastion://act?a=rm_dl&id=" << esc(d.id) << "\">Remove</a>";
            }
            o << "</div>";
            o << "</div>"; // row
        }
        o << "</div>";
    } else {
        o << "<div class=\"card\" style=\"text-align:center;padding:40px\">"
          << "<p class=\"dim\">When you download a file, it'll appear here. "
             "Files save to <code>~/Downloads/</code>.</p>"
          << "</div>";
    }

    o << "</div></body></html>";
    return o.str();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Blocked page
// ─────────────────────────────────────────────────────────────────────────────
std::string blocked(const std::string& url) {
    std::ostringstream o;
    o << "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Blocked</title>"
      << "<style>body{background:#0f0f14;color:#ff6b6b;font-family:monospace;"
         "display:flex;align-items:center;justify-content:center;height:100vh;margin:0}"
         "div{text-align:center;max-width:600px;padding:20px}"
         "h1{font-size:24px;margin-bottom:12px}"
         "code{background:#1c1c24;padding:4px 8px;border-radius:4px;word-break:break-all}"
         "</style></head><body><div>"
      << "<h1>🛡 Request blocked</h1>"
      << "<p>Bastion blocked a tracker or advertising request.</p>"
      << "<p style=\"margin-top:16px\"><code>" << esc(url) << "</code></p>"
      << "</div></body></html>";
    return o.str();
}

std::string not_found(const std::string& path) {
    std::ostringstream o;
    o << "<!DOCTYPE html><html><body style=\"font-family:monospace;padding:40px\">"
      << "<h1>bastion://" << esc(path) << " — not found</h1>"
      << "<p><a href=\"bastion://home\">Back to home</a></p>"
      << "</body></html>";
    return o.str();
}

} // namespace internal_pages
