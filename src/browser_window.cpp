#include "browser_window.h"
#include "themes.h"
#include "internal_pages.h"
#include "download_manager.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <glib.h>

static const char* DEFAULT_HOME = "bastion://home";

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────
BrowserWindow::BrowserWindow(GtkApplication* app,
                             std::shared_ptr<PrivacyEngine> engine,
                             std::shared_ptr<AppState>      state)
    : engine_(std::move(engine))
    , state_ (std::move(state))
    , downloads_(std::make_unique<DownloadManager>())
{
    shared_ctx_ = engine_->create_context();

    // Attach download manager to the shared context
    downloads_->attach(shared_ctx_);
    downloads_->set_change_callback(
        [](void* ud){
            auto* bw = static_cast<BrowserWindow*>(ud);
            bw->update_downloads_button();
        }, this);

    register_bastion_scheme();
    build_ui(app);
    apply_theme();
    open_tab(DEFAULT_HOME);
    gtk_widget_show_all(window_);
    gtk_widget_hide(find_bar_);
}

BrowserWindow::~BrowserWindow() {
    if (shared_ctx_)   g_object_unref(shared_ctx_);
    if (css_provider_) g_object_unref(css_provider_);
}

// ─────────────────────────────────────────────────────────────────────────────
//  UI
// ─────────────────────────────────────────────────────────────────────────────
void BrowserWindow::build_ui(GtkApplication* app) {
    window_ = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window_), "Bastion Browser");
    gtk_window_set_default_size(GTK_WINDOW(window_), 1280, 900);

    // Keyboard shortcuts
    g_signal_connect(window_, "key-press-event",
                     G_CALLBACK(on_key_press), this);

    main_box_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window_), main_box_);

    build_toolbar();

    notebook_ = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook_), TRUE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook_), FALSE);
    gtk_widget_set_vexpand(notebook_, TRUE);
    gtk_box_pack_start(GTK_BOX(main_box_), notebook_, TRUE, TRUE, 0);
    g_signal_connect(notebook_, "switch-page",
                     G_CALLBACK(on_switch_page), this);

    build_find_bar();
    build_statusbar();
}

void BrowserWindow::build_toolbar() {
    toolbar_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_style_context_add_class(gtk_widget_get_style_context(toolbar_), "toolbar-box");
    gtk_box_pack_start(GTK_BOX(main_box_), toolbar_, FALSE, FALSE, 0);

    btn_back_     = gtk_button_new_with_label("◀");
    btn_forward_  = gtk_button_new_with_label("▶");
    btn_reload_   = gtk_button_new_with_label("↺");
    btn_home_     = gtk_button_new_with_label("🏠");
    btn_new_tab_  = gtk_button_new_with_label("+");

    gtk_widget_set_tooltip_text(btn_back_,    "Back (Alt+←)");
    gtk_widget_set_tooltip_text(btn_forward_, "Forward (Alt+→)");
    gtk_widget_set_tooltip_text(btn_reload_,  "Reload (F5)");
    gtk_widget_set_tooltip_text(btn_home_,    "Home");
    gtk_widget_set_tooltip_text(btn_new_tab_, "New Tab (Ctrl+T)");

    g_signal_connect(btn_back_,    "clicked", G_CALLBACK(on_back_clicked),    this);
    g_signal_connect(btn_forward_, "clicked", G_CALLBACK(on_forward_clicked), this);
    g_signal_connect(btn_reload_,  "clicked", G_CALLBACK(on_reload_clicked),  this);
    g_signal_connect(btn_home_,    "clicked", G_CALLBACK(on_home_clicked),    this);
    g_signal_connect(btn_new_tab_, "clicked", G_CALLBACK(on_new_tab_clicked), this);

    gtk_box_pack_start(GTK_BOX(toolbar_), btn_back_,    FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_), btn_forward_, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_), btn_reload_,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_), btn_home_,    FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_), btn_new_tab_, FALSE, FALSE, 0);

    // Address bar
    address_entry_ = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(address_entry_),
        "Search or enter URL — Ctrl+L to focus");
    gtk_widget_set_hexpand(address_entry_, TRUE);
    g_signal_connect(address_entry_, "activate",
                     G_CALLBACK(on_address_activate), this);
    gtk_box_pack_start(GTK_BOX(toolbar_), address_entry_, TRUE, TRUE, 4);

    btn_go_ = gtk_button_new_with_label("Go");
    g_signal_connect(btn_go_, "clicked", G_CALLBACK(on_go_clicked), this);
    gtk_box_pack_start(GTK_BOX(toolbar_), btn_go_, FALSE, FALSE, 0);

    // Bookmark toggle
    btn_bookmark_ = gtk_button_new_with_label("☆");
    gtk_widget_set_tooltip_text(btn_bookmark_, "Bookmark this page (Ctrl+D)");
    g_signal_connect(btn_bookmark_, "clicked",
                     G_CALLBACK(on_bookmark_clicked), this);
    gtk_box_pack_start(GTK_BOX(toolbar_), btn_bookmark_, FALSE, FALSE, 0);

    // Downloads button
    btn_downloads_ = gtk_button_new_with_label("⬇");
    gtk_widget_set_tooltip_text(btn_downloads_, "Downloads (Ctrl+J)");
    g_signal_connect(btn_downloads_, "clicked",
                     G_CALLBACK(on_downloads_clicked), this);
    gtk_box_pack_start(GTK_BOX(toolbar_), btn_downloads_, FALSE, FALSE, 0);

    // Menu button
    btn_menu_ = gtk_button_new_with_label("☰");
    gtk_widget_set_tooltip_text(btn_menu_, "Menu");
    g_signal_connect(btn_menu_, "clicked", G_CALLBACK(on_menu_clicked), this);
    gtk_box_pack_start(GTK_BOX(toolbar_), btn_menu_, FALSE, FALSE, 0);
}

void BrowserWindow::build_find_bar() {
    find_bar_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_style_context_add_class(gtk_widget_get_style_context(find_bar_), "toolbar-box");
    gtk_box_pack_start(GTK_BOX(main_box_), find_bar_, FALSE, FALSE, 0);

    GtkWidget* label = gtk_label_new("Find:");
    gtk_box_pack_start(GTK_BOX(find_bar_), label, FALSE, FALSE, 4);

    find_entry_ = gtk_entry_new();
    gtk_widget_set_hexpand(find_entry_, TRUE);
    g_signal_connect(find_entry_, "activate",
                     G_CALLBACK(on_find_activate), this);
    gtk_box_pack_start(GTK_BOX(find_bar_), find_entry_, TRUE, TRUE, 0);

    GtkWidget* prev = gtk_button_new_with_label("▲");
    GtkWidget* next = gtk_button_new_with_label("▼");
    GtkWidget* clo  = gtk_button_new_with_label("✕");
    g_signal_connect(prev, "clicked", G_CALLBACK(on_find_prev_clicked),  this);
    g_signal_connect(next, "clicked", G_CALLBACK(on_find_next_clicked),  this);
    g_signal_connect(clo,  "clicked", G_CALLBACK(on_find_close_clicked), this);
    gtk_box_pack_start(GTK_BOX(find_bar_), prev, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(find_bar_), next, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(find_bar_), clo,  FALSE, FALSE, 0);
}

void BrowserWindow::build_statusbar() {
    GtkWidget* bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(bar), "statusbar");
    gtk_box_pack_end(GTK_BOX(main_box_), bar, FALSE, FALSE, 0);

    proxy_label_ = gtk_label_new("");
    std::string pdesc = "[🧅 " + engine_->proxy().description() + "]";
    gtk_label_set_text(GTK_LABEL(proxy_label_), pdesc.c_str());
    gtk_style_context_add_class(gtk_widget_get_style_context(proxy_label_),
        engine_->proxy().is_active() ? "proxy-active" : "proxy-none");
    gtk_box_pack_start(GTK_BOX(bar), proxy_label_, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(bar), gtk_label_new("│"), FALSE, FALSE, 0);

    tls_label_ = gtk_label_new("[🔒 Ready]");
    gtk_box_pack_start(GTK_BOX(bar), tls_label_, FALSE, FALSE, 0);

    GtkWidget* spacer = gtk_label_new("");
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_pack_start(GTK_BOX(bar), spacer, TRUE, TRUE, 0);

    gtk_box_pack_end(GTK_BOX(bar), gtk_label_new("[Ephemeral · No disk writes]"),
                     FALSE, FALSE, 4);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Theme
// ─────────────────────────────────────────────────────────────────────────────
void BrowserWindow::apply_theme() {
    const char* css = themes::MODERN_DARK;
    switch (state_->theme) {
        case Theme::VISTA:        css = themes::VISTA;        break;
        case Theme::MODERN_DARK:  css = themes::MODERN_DARK;  break;
        case Theme::MODERN_LIGHT: css = themes::MODERN_LIGHT; break;
    }

    if (!css_provider_) {
        css_provider_ = gtk_css_provider_new();
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(css_provider_),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
    gtk_css_provider_load_from_data(css_provider_, css, -1, nullptr);

    // Reload all currently-open bastion:// pages so their inline CSS updates too
    for (auto& t : tabs_) {
        const gchar* u = webkit_web_view_get_uri(t.webview);
        if (u && std::string(u).rfind("bastion://", 0) == 0)
            webkit_web_view_reload(t.webview);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  bastion:// URI scheme
// ─────────────────────────────────────────────────────────────────────────────
void BrowserWindow::register_bastion_scheme() {
    webkit_web_context_register_uri_scheme(
        shared_ctx_, "bastion",
        on_bastion_scheme, this, nullptr);
}

void BrowserWindow::on_bastion_scheme(WebKitURISchemeRequest* req, gpointer user_data) {
    auto* bw = static_cast<BrowserWindow*>(user_data);
    bw->handle_bastion_request(req);
}

static void respond_html(WebKitURISchemeRequest* req, const std::string& html) {
    // Copy the HTML into a buffer owned by a GBytes so it lives past this call
    GBytes* bytes = g_bytes_new(html.data(), html.size());
    GInputStream* stream = g_memory_input_stream_new_from_bytes(bytes);
    webkit_uri_scheme_request_finish(req, stream,
                                     static_cast<gint64>(html.size()),
                                     "text/html; charset=utf-8");
    g_object_unref(stream);
    g_bytes_unref(bytes);
}

// Parse "key=val&key2=val2" — super simple, no URL-decoding (tolerable for our use)
static std::string get_param(const std::string& query, const std::string& key) {
    std::string pat = key + "=";
    auto pos = query.find(pat);
    if (pos == std::string::npos) return {};
    pos += pat.size();
    auto end = query.find('&', pos);
    return query.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
}

// Minimal URL-decoder (%XX and + → space)
static std::string url_decode(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '+') out += ' ';
        else if (s[i] == '%' && i + 2 < s.size()) {
            int hi = 0, lo = 0;
            auto hex = [](char c){
                if (c>='0'&&c<='9') return c-'0';
                if (c>='a'&&c<='f') return c-'a'+10;
                if (c>='A'&&c<='F') return c-'A'+10;
                return 0;
            };
            hi = hex(s[i+1]); lo = hex(s[i+2]);
            out += static_cast<char>((hi<<4)|lo);
            i += 2;
        } else out += s[i];
    }
    return out;
}

void BrowserWindow::handle_bastion_request(WebKitURISchemeRequest* req) {
    const gchar* uri = webkit_uri_scheme_request_get_uri(req);
    if (!uri) { respond_html(req, internal_pages::not_found("")); return; }

    std::string u(uri);
    // Strip "bastion://"
    std::string rest = u.substr(10);
    std::string path, query;
    auto qpos = rest.find('?');
    if (qpos != std::string::npos) {
        path  = rest.substr(0, qpos);
        query = rest.substr(qpos + 1);
    } else path = rest;

    // Trim trailing slash
    if (!path.empty() && path.back() == '/') path.pop_back();

    if (path == "home" || path.empty()) {
        respond_html(req, internal_pages::home(*state_));
    } else if (path == "settings") {
        respond_html(req, internal_pages::settings(*state_));
    } else if (path == "bookmarks") {
        respond_html(req, internal_pages::bookmarks(*state_));
    } else if (path == "downloads") {
        respond_html(req, internal_pages::downloads(*state_, *downloads_));
    } else if (path == "search") {
        // bastion://search?q=xxx → navigate to real search URL
        std::string q = url_decode(get_param(query, "q"));
        std::string target = q.empty() ? "bastion://home" : sanitize_uri(q);
        // Respond with a redirect page
        std::string html = "<!DOCTYPE html><meta http-equiv=\"refresh\" "
                           "content=\"0;url=" + target + "\">";
        respond_html(req, html);
    } else if (path == "act") {
        handle_bastion_action(req, query);
    } else if (path == "blocked") {
        std::string u2 = url_decode(get_param(query, "u"));
        respond_html(req, internal_pages::blocked(u2));
    } else {
        respond_html(req, internal_pages::not_found(path));
    }
}

void BrowserWindow::handle_bastion_action(WebKitURISchemeRequest* req,
                                          const std::string& query)
{
    std::string action  = get_param(query, "a");
    std::string value   = get_param(query, "v");
    std::string redirect_to = "bastion://settings";

    if (action == "theme") {
        try { state_->theme = static_cast<Theme>(std::stoi(value)); } catch (...) {}
        state_->save();
        // Schedule theme apply on main loop (safe to do now since we're on main thread)
        apply_theme();
        redirect_to = "bastion://settings";

    } else if (action == "search") {
        try { state_->search = static_cast<SearchEngine>(std::stoi(value)); } catch (...) {}
        state_->save();
        redirect_to = "bastion://settings";

    } else if (action == "toggle_bookmarks") {
        state_->persist_bookmarks = !state_->persist_bookmarks;
        state_->save();
        redirect_to = "bastion://settings";

    } else if (action == "clear_bookmarks") {
        state_->bookmarks.clear();
        state_->save();
        redirect_to = "bastion://bookmarks";

    } else if (action == "rm_bookmark") {
        std::string u = url_decode(get_param(query, "u"));
        state_->remove_bookmark(u);
        redirect_to = "bastion://bookmarks";

    } else if (action == "cancel_dl") {
        std::string id = url_decode(get_param(query, "id"));
        downloads_->cancel(id);
        redirect_to = "bastion://downloads";

    } else if (action == "rm_dl") {
        std::string id = url_decode(get_param(query, "id"));
        downloads_->remove(id);
        redirect_to = "bastion://downloads";

    } else if (action == "clear_downloads") {
        downloads_->clear_completed();
        redirect_to = "bastion://downloads";
    }

    // Issue a meta-refresh redirect
    std::string html = "<!DOCTYPE html><meta http-equiv=\"refresh\" content=\"0;url="
                     + redirect_to + "\"><body style=\"background:#111\"></body>";
    respond_html(req, html);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tab management
// ─────────────────────────────────────────────────────────────────────────────
Tab BrowserWindow::create_tab(const std::string& uri) {
    Tab tab{};

    WebKitUserContentManager* ucm = engine_->create_content_manager(shared_ctx_);
    WebKitSettings*           settings = engine_->create_settings();

    tab.webview = WEBKIT_WEB_VIEW(g_object_new(
        WEBKIT_TYPE_WEB_VIEW,
        "web-context", shared_ctx_,
        "user-content-manager", ucm,
        "settings", settings,
        nullptr));

    g_object_unref(ucm);
    g_object_unref(settings);

    g_signal_connect(tab.webview, "load-changed",                G_CALLBACK(on_load_changed),             this);
    g_signal_connect(tab.webview, "notify::title",               G_CALLBACK(on_notify_title),             this);
    g_signal_connect(tab.webview, "notify::uri",                 G_CALLBACK(on_notify_uri),               this);
    g_signal_connect(tab.webview, "decide-policy",               G_CALLBACK(on_decide_policy),            this);
    g_signal_connect(tab.webview, "load-failed-with-tls-errors", G_CALLBACK(on_tls_error),                this);
    g_signal_connect(tab.webview, "notify::estimated-load-progress",
                     G_CALLBACK(on_notify_estimated_progress), this);

    tab.label_box  = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    tab.label_text = gtk_label_new("New Tab");
    gtk_label_set_max_width_chars(GTK_LABEL(tab.label_text), 20);
    gtk_label_set_ellipsize(GTK_LABEL(tab.label_text), PANGO_ELLIPSIZE_END);
    gtk_style_context_add_class(gtk_widget_get_style_context(tab.label_text), "tab-label");

    GtkWidget* close_btn = gtk_button_new_with_label("✕");
    gtk_button_set_relief(GTK_BUTTON(close_btn), GTK_RELIEF_NONE);
    gtk_widget_set_focus_on_click(close_btn, FALSE);
    g_object_set_data(G_OBJECT(close_btn), "webview", tab.webview);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_close_tab_clicked), this);

    gtk_box_pack_start(GTK_BOX(tab.label_box), tab.label_text, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(tab.label_box), close_btn, FALSE, FALSE, 0);
    gtk_widget_show_all(tab.label_box);

    tab.uri = uri;
    return tab;
}

void BrowserWindow::open_tab(const std::string& uri) {
    Tab tab = create_tab(uri);
    GtkWidget* webview_widget = GTK_WIDGET(tab.webview);
    gtk_widget_set_vexpand(webview_widget, TRUE);
    gtk_widget_set_hexpand(webview_widget, TRUE);

    int page_num = gtk_notebook_append_page(
        GTK_NOTEBOOK(notebook_), webview_widget, tab.label_box);
    gtk_widget_show_all(webview_widget);
    tabs_.push_back(tab);

    webkit_web_view_load_uri(tab.webview, uri.c_str());
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_), page_num);
    update_nav_buttons();
}

void BrowserWindow::close_tab(int page_num) {
    if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook_)) <= 1) {
        if (!tabs_.empty())
            webkit_web_view_load_uri(tabs_[0].webview, DEFAULT_HOME);
        return;
    }
    GtkWidget* page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook_), page_num);
    for (auto it = tabs_.begin(); it != tabs_.end(); ++it) {
        if (GTK_WIDGET(it->webview) == page) { tabs_.erase(it); break; }
    }
    gtk_notebook_remove_page(GTK_NOTEBOOK(notebook_), page_num);
}

Tab* BrowserWindow::current_tab() {
    int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook_));
    if (page < 0) return nullptr;
    GtkWidget* wv = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook_), page);
    for (auto& t : tabs_) if (GTK_WIDGET(t.webview) == wv) return &t;
    return nullptr;
}

int BrowserWindow::tab_index_of(WebKitWebView* wv) {
    for (int i = 0; i < gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook_)); i++)
        if (gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook_), i) == GTK_WIDGET(wv))
            return i;
    return -1;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Navigation
// ─────────────────────────────────────────────────────────────────────────────
std::string BrowserWindow::sanitize_uri(const std::string& input) const {
    if (input.empty()) return DEFAULT_HOME;

    if (input.rfind("http://",    0) == 0 ||
        input.rfind("https://",   0) == 0 ||
        input.rfind("about:",     0) == 0 ||
        input.rfind("bastion://", 0) == 0 ||
        input.rfind("file://",    0) == 0)
        return input;

    if (input.find('.') != std::string::npos && input.find(' ') == std::string::npos)
        return "https://" + input;

    return state_->search_url(input);
}

void BrowserWindow::navigate(const std::string& input) {
    Tab* tab = current_tab();
    if (!tab) return;
    std::string uri = sanitize_uri(input);
    tab->uri = uri;
    webkit_web_view_load_uri(tab->webview, uri.c_str());
}

// ─────────────────────────────────────────────────────────────────────────────
//  UI updates
// ─────────────────────────────────────────────────────────────────────────────
void BrowserWindow::update_nav_buttons() {
    Tab* tab = current_tab();
    if (!tab) return;
    gtk_widget_set_sensitive(btn_back_,    webkit_web_view_can_go_back   (tab->webview));
    gtk_widget_set_sensitive(btn_forward_, webkit_web_view_can_go_forward(tab->webview));
    update_bookmark_button();
}

void BrowserWindow::update_address_bar(const std::string& uri) {
    gtk_entry_set_text(GTK_ENTRY(address_entry_), uri.c_str());
}

void BrowserWindow::update_bookmark_button() {
    Tab* tab = current_tab();
    if (!tab) return;
    const gchar* u = webkit_web_view_get_uri(tab->webview);
    bool marked = u && state_->is_bookmarked(u);
    gtk_button_set_label(GTK_BUTTON(btn_bookmark_), marked ? "★" : "☆");
}

void BrowserWindow::update_downloads_button() {
    if (!btn_downloads_ || !downloads_) return;
    int active = downloads_->active_count();
    if (active > 0) {
        std::ostringstream s;
        s << "⬇ " << active;
        gtk_button_set_label(GTK_BUTTON(btn_downloads_), s.str().c_str());
    } else {
        gtk_button_set_label(GTK_BUTTON(btn_downloads_), "⬇");
    }

    // Live-refresh the downloads page if that's the current tab
    Tab* tab = current_tab();
    if (tab) {
        const gchar* u = webkit_web_view_get_uri(tab->webview);
        if (u && std::string(u) == "bastion://downloads")
            webkit_web_view_reload(tab->webview);
    }
}

void BrowserWindow::update_tls_status(WebKitWebView* wv) {
    GTlsCertificate* cert = nullptr;
    GTlsCertificateFlags flags;
    const gchar* u = webkit_web_view_get_uri(wv);
    bool is_bastion = u && std::string(u).rfind("bastion://", 0) == 0;

    if (is_bastion) {
        gtk_label_set_text(GTK_LABEL(tls_label_), "[🔐 Internal]");
        GtkStyleContext* sc = gtk_widget_get_style_context(tls_label_);
        gtk_style_context_remove_class(sc, "tls-warn");
        gtk_style_context_add_class(sc, "tls-ok");
        return;
    }

    bool has_tls = webkit_web_view_get_tls_info(wv, &cert, &flags);
    std::string status;
    GtkStyleContext* sc = gtk_widget_get_style_context(tls_label_);
    if (!has_tls) {
        status = "[⚠ No TLS]";
        gtk_style_context_remove_class(sc, "tls-ok");
        gtk_style_context_add_class(sc, "tls-warn");
    } else if (flags != 0) {
        status = "[⚠ TLS Error]";
        gtk_style_context_remove_class(sc, "tls-ok");
        gtk_style_context_add_class(sc, "tls-warn");
    } else {
        status = "[🔒 TLS OK]";
        gtk_style_context_remove_class(sc, "tls-warn");
        gtk_style_context_add_class(sc, "tls-ok");
    }
    gtk_label_set_text(GTK_LABEL(tls_label_), status.c_str());
}

void BrowserWindow::update_tab_title(WebKitWebView* wv, const std::string& title) {
    for (auto& tab : tabs_) {
        if (tab.webview == wv) {
            std::string display = title.empty() ? "New Tab" : title;
            gtk_label_set_text(GTK_LABEL(tab.label_text), display.c_str());
            Tab* cur = current_tab();
            if (cur && cur->webview == wv)
                gtk_window_set_title(GTK_WINDOW(window_),
                    (display + " – Bastion").c_str());
            return;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Find in page
// ─────────────────────────────────────────────────────────────────────────────
void BrowserWindow::find_show() {
    find_visible_ = true;
    gtk_widget_show(find_bar_);
    gtk_widget_grab_focus(find_entry_);
}

void BrowserWindow::find_hide() {
    find_visible_ = false;
    gtk_widget_hide(find_bar_);
    Tab* t = current_tab();
    if (t) {
        WebKitFindController* fc = webkit_web_view_get_find_controller(t->webview);
        webkit_find_controller_search_finish(fc);
    }
}

void BrowserWindow::find_next() {
    Tab* t = current_tab();
    if (!t) return;
    const gchar* q = gtk_entry_get_text(GTK_ENTRY(find_entry_));
    if (!q || !*q) return;
    WebKitFindController* fc = webkit_web_view_get_find_controller(t->webview);
    webkit_find_controller_search(fc, q,
        WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE | WEBKIT_FIND_OPTIONS_WRAP_AROUND, 100);
}

void BrowserWindow::find_prev() {
    Tab* t = current_tab();
    if (!t) return;
    const gchar* q = gtk_entry_get_text(GTK_ENTRY(find_entry_));
    if (!q || !*q) return;
    WebKitFindController* fc = webkit_web_view_get_find_controller(t->webview);
    webkit_find_controller_search(fc, q,
        WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE | WEBKIT_FIND_OPTIONS_WRAP_AROUND |
        WEBKIT_FIND_OPTIONS_BACKWARDS, 100);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Zoom
// ─────────────────────────────────────────────────────────────────────────────
void BrowserWindow::zoom_in() {
    Tab* t = current_tab(); if (!t) return;
    webkit_web_view_set_zoom_level(t->webview,
        webkit_web_view_get_zoom_level(t->webview) + 0.1);
}
void BrowserWindow::zoom_out() {
    Tab* t = current_tab(); if (!t) return;
    double z = webkit_web_view_get_zoom_level(t->webview) - 0.1;
    if (z < 0.2) z = 0.2;
    webkit_web_view_set_zoom_level(t->webview, z);
}
void BrowserWindow::zoom_reset() {
    Tab* t = current_tab(); if (!t) return;
    webkit_web_view_set_zoom_level(t->webview, 1.0);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Bookmark toggle
// ─────────────────────────────────────────────────────────────────────────────
void BrowserWindow::toggle_bookmark_current() {
    Tab* t = current_tab(); if (!t) return;
    const gchar* u = webkit_web_view_get_uri(t->webview);
    if (!u) return;
    std::string url(u);
    if (url.rfind("bastion://", 0) == 0) return;

    if (state_->is_bookmarked(url)) {
        state_->remove_bookmark(url);
    } else {
        const gchar* title = webkit_web_view_get_title(t->webview);
        state_->add_bookmark(url, title ? title : url);
    }
    update_bookmark_button();
}

// ─────────────────────────────────────────────────────────────────────────────
//  GTK callbacks
// ─────────────────────────────────────────────────────────────────────────────
void BrowserWindow::on_go_clicked(GtkWidget*, gpointer self) {
    auto* bw = static_cast<BrowserWindow*>(self);
    const gchar* text = gtk_entry_get_text(GTK_ENTRY(bw->address_entry_));
    bw->navigate(text ? text : "");
}

void BrowserWindow::on_address_activate(GtkWidget*, gpointer self) {
    on_go_clicked(nullptr, self);
}

void BrowserWindow::on_back_clicked(GtkWidget*, gpointer self) {
    auto* bw = static_cast<BrowserWindow*>(self);
    Tab* t = bw->current_tab();
    if (t) webkit_web_view_go_back(t->webview);
}

void BrowserWindow::on_forward_clicked(GtkWidget*, gpointer self) {
    auto* bw = static_cast<BrowserWindow*>(self);
    Tab* t = bw->current_tab();
    if (t) webkit_web_view_go_forward(t->webview);
}

void BrowserWindow::on_reload_clicked(GtkWidget*, gpointer self) {
    auto* bw = static_cast<BrowserWindow*>(self);
    Tab* t = bw->current_tab();
    if (t) webkit_web_view_reload(t->webview);
}

void BrowserWindow::on_home_clicked(GtkWidget*, gpointer self) {
    auto* bw = static_cast<BrowserWindow*>(self);
    Tab* t = bw->current_tab();
    if (t) webkit_web_view_load_uri(t->webview, DEFAULT_HOME);
}

void BrowserWindow::on_new_tab_clicked(GtkWidget*, gpointer self) {
    auto* bw = static_cast<BrowserWindow*>(self);
    bw->open_tab(DEFAULT_HOME);
}

void BrowserWindow::on_bookmark_clicked(GtkWidget*, gpointer self) {
    static_cast<BrowserWindow*>(self)->toggle_bookmark_current();
}

void BrowserWindow::on_downloads_clicked(GtkWidget*, gpointer self) {
    auto* bw = static_cast<BrowserWindow*>(self);
    Tab* t = bw->current_tab();
    if (t) webkit_web_view_load_uri(t->webview, "bastion://downloads");
}

void BrowserWindow::on_menu_clicked(GtkWidget* btn, gpointer self) {
    auto* bw = static_cast<BrowserWindow*>(self);
    GtkWidget* menu = gtk_menu_new();

    auto add_item = [&](const char* label, const char* uri) {
        GtkWidget* item = gtk_menu_item_new_with_label(label);
        g_object_set_data_full(G_OBJECT(item), "uri", g_strdup(uri), g_free);
        g_signal_connect(item, "activate",
            G_CALLBACK(+[](GtkMenuItem* mi, gpointer d){
                auto* bw2 = static_cast<BrowserWindow*>(d);
                const char* u = static_cast<const char*>(g_object_get_data(G_OBJECT(mi), "uri"));
                Tab* t = bw2->current_tab();
                if (t && u) webkit_web_view_load_uri(t->webview, u);
            }), bw);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    };

    add_item("🏠 Home",       "bastion://home");
    add_item("⭐ Bookmarks",  "bastion://bookmarks");
    add_item("⬇ Downloads",   "bastion://downloads");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    add_item("⚙ Settings",    "bastion://settings");

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_widget(GTK_MENU(menu), btn,
        GDK_GRAVITY_SOUTH_EAST, GDK_GRAVITY_NORTH_EAST, nullptr);
}

void BrowserWindow::on_switch_page(GtkNotebook*, GtkWidget* page, guint, gpointer self) {
    auto* bw = static_cast<BrowserWindow*>(self);
    for (auto& tab : bw->tabs_) {
        if (GTK_WIDGET(tab.webview) == page) {
            const gchar* uri = webkit_web_view_get_uri(tab.webview);
            bw->update_address_bar(uri ? uri : "");
            bw->update_nav_buttons();
            bw->update_tls_status(tab.webview);
            const gchar* title = webkit_web_view_get_title(tab.webview);
            if (title) {
                std::string t(title);
                gtk_window_set_title(GTK_WINDOW(bw->window_), (t + " – Bastion").c_str());
            }
            return;
        }
    }
}

void BrowserWindow::on_close_tab_clicked(GtkWidget* btn, gpointer self) {
    auto* bw = static_cast<BrowserWindow*>(self);
    WebKitWebView* wv = WEBKIT_WEB_VIEW(g_object_get_data(G_OBJECT(btn), "webview"));
    int idx = bw->tab_index_of(wv);
    if (idx >= 0) bw->close_tab(idx);
}

// Find bar callbacks
void BrowserWindow::on_find_activate    (GtkWidget*, gpointer self) { static_cast<BrowserWindow*>(self)->find_next(); }
void BrowserWindow::on_find_next_clicked(GtkWidget*, gpointer self) { static_cast<BrowserWindow*>(self)->find_next(); }
void BrowserWindow::on_find_prev_clicked(GtkWidget*, gpointer self) { static_cast<BrowserWindow*>(self)->find_prev(); }
void BrowserWindow::on_find_close_clicked(GtkWidget*, gpointer self){ static_cast<BrowserWindow*>(self)->find_hide();  }

// ─────────────────────────────────────────────────────────────────────────────
//  WebKit callbacks
// ─────────────────────────────────────────────────────────────────────────────
void BrowserWindow::on_load_changed(WebKitWebView* wv, WebKitLoadEvent event, gpointer self) {
    auto* bw = static_cast<BrowserWindow*>(self);
    Tab* cur = bw->current_tab();
    bool is_current = cur && cur->webview == wv;

    switch (event) {
    case WEBKIT_LOAD_STARTED:
        if (is_current) gtk_label_set_text(GTK_LABEL(bw->tls_label_), "[🔄 Loading...]");
        break;
    case WEBKIT_LOAD_COMMITTED:
        if (is_current) {
            const gchar* uri = webkit_web_view_get_uri(wv);
            bw->update_address_bar(uri ? uri : "");
            bw->update_tls_status(wv);
            bw->update_bookmark_button();
        }
        break;
    case WEBKIT_LOAD_FINISHED:
        if (is_current) {
            bw->update_nav_buttons();
            bw->update_tls_status(wv);
        }
        // No history recording — true privacy.
        break;
    default: break;
    }
}

void BrowserWindow::on_notify_title(WebKitWebView* wv, GParamSpec*, gpointer self) {
    auto* bw = static_cast<BrowserWindow*>(self);
    const gchar* title = webkit_web_view_get_title(wv);
    bw->update_tab_title(wv, title ? title : "");
}

void BrowserWindow::on_notify_uri(WebKitWebView* wv, GParamSpec*, gpointer self) {
    auto* bw = static_cast<BrowserWindow*>(self);
    Tab* cur = bw->current_tab();
    if (cur && cur->webview == wv) {
        const gchar* uri = webkit_web_view_get_uri(wv);
        bw->update_address_bar(uri ? uri : "");
        bw->update_bookmark_button();
    }
}

gboolean BrowserWindow::on_decide_policy(WebKitWebView* wv, WebKitPolicyDecision* decision,
                                          WebKitPolicyDecisionType type, gpointer self)
{
    auto* bw = static_cast<BrowserWindow*>(self);

    if (type == WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION) {
        auto* nav = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
        auto* action = webkit_navigation_policy_decision_get_navigation_action(nav);
        auto* req = webkit_navigation_action_get_request(action);
        const gchar* uri = webkit_uri_request_get_uri(req);
        if (uri) {
            std::string u(uri);
            if (u.rfind("http://", 0) == 0) {
                std::string secure = "https://" + u.substr(7);
                webkit_web_view_load_uri(wv, secure.c_str());
                webkit_policy_decision_ignore(decision);
                return TRUE;
            }
        }
    }

    if (type == WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION) {
        auto* nav = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
        auto* action = webkit_navigation_policy_decision_get_navigation_action(nav);
        auto* req = webkit_navigation_action_get_request(action);
        const gchar* uri = webkit_uri_request_get_uri(req);
        if (uri) bw->open_tab(uri);
        webkit_policy_decision_ignore(decision);
        return TRUE;
    }

    webkit_policy_decision_use(decision);
    return TRUE;
}

gboolean BrowserWindow::on_tls_error(WebKitWebView* wv, const gchar* failing_uri,
                                      GTlsCertificate*, GTlsCertificateFlags flags, gpointer self)
{
    auto* bw = static_cast<BrowserWindow*>(self);
    std::cerr << "[Bastion] TLS error (" << flags << ") for: " << failing_uri << "\n";

    const char* page =
        "<html><head><style>"
        "body{background:#0f0f14;color:#ff6b6b;font-family:monospace;"
        "display:flex;align-items:center;justify-content:center;height:100vh;margin:0}"
        "div{text-align:center;max-width:600px;padding:20px}"
        "h1{color:#ff6b6b;font-size:1.8em;margin-bottom:16px}"
        "p{color:#aaa;line-height:1.6}"
        "</style></head><body><div>"
        "<h1>🔴 TLS Certificate Error</h1>"
        "<p>Bastion refused to connect because the server's TLS certificate is invalid.</p>"
        "<p>This could mean someone is intercepting your connection.</p>"
        "</div></body></html>";
    webkit_web_view_load_html(wv, page, nullptr);

    Tab* cur = bw->current_tab();
    if (cur && cur->webview == wv) {
        gtk_label_set_text(GTK_LABEL(bw->tls_label_), "[🔴 TLS FAILED]");
        GtkStyleContext* sc = gtk_widget_get_style_context(bw->tls_label_);
        gtk_style_context_remove_class(sc, "tls-ok");
        gtk_style_context_add_class(sc, "tls-warn");
    }
    return TRUE;
}

void BrowserWindow::on_notify_estimated_progress(WebKitWebView* wv, GParamSpec*, gpointer self) {
    auto* bw = static_cast<BrowserWindow*>(self);
    Tab* cur = bw->current_tab();
    if (!cur || cur->webview != wv) return;
    double progress = webkit_web_view_get_estimated_load_progress(wv);
    if (progress < 1.0) {
        std::ostringstream s;
        s << "[🔄 " << static_cast<int>(progress * 100) << "%]";
        gtk_label_set_text(GTK_LABEL(bw->tls_label_), s.str().c_str());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Keyboard shortcuts
// ─────────────────────────────────────────────────────────────────────────────
gboolean BrowserWindow::on_key_press(GtkWidget*, GdkEventKey* ev, gpointer self) {
    auto* bw = static_cast<BrowserWindow*>(self);
    guint key = ev->keyval;
    guint mods = ev->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK | GDK_MOD1_MASK);

    // Escape closes find bar
    if (key == GDK_KEY_Escape && bw->find_visible_) { bw->find_hide(); return TRUE; }

    if (mods == GDK_CONTROL_MASK) {
        switch (key) {
            case GDK_KEY_t: bw->open_tab(DEFAULT_HOME); return TRUE;
            case GDK_KEY_w: {
                int p = gtk_notebook_get_current_page(GTK_NOTEBOOK(bw->notebook_));
                if (p >= 0) bw->close_tab(p);
                return TRUE;
            }
            case GDK_KEY_l:
                gtk_widget_grab_focus(bw->address_entry_);
                gtk_editable_select_region(GTK_EDITABLE(bw->address_entry_), 0, -1);
                return TRUE;
            case GDK_KEY_r: on_reload_clicked(nullptr, bw); return TRUE;
            case GDK_KEY_f: bw->find_show(); return TRUE;
            case GDK_KEY_d: bw->toggle_bookmark_current(); return TRUE;
            case GDK_KEY_j: {
                Tab* t = bw->current_tab();
                if (t) webkit_web_view_load_uri(t->webview, "bastion://downloads");
                return TRUE;
            }
            case GDK_KEY_comma: {
                Tab* t = bw->current_tab();
                if (t) webkit_web_view_load_uri(t->webview, "bastion://settings");
                return TRUE;
            }
            case GDK_KEY_plus: case GDK_KEY_equal: bw->zoom_in();    return TRUE;
            case GDK_KEY_minus:                   bw->zoom_out();   return TRUE;
            case GDK_KEY_0:                       bw->zoom_reset(); return TRUE;
        }
    }

    if (mods == GDK_MOD1_MASK) { // Alt
        if (key == GDK_KEY_Left)  { on_back_clicked   (nullptr, bw); return TRUE; }
        if (key == GDK_KEY_Right) { on_forward_clicked(nullptr, bw); return TRUE; }
    }

    if (mods == 0 && key == GDK_KEY_F5) { on_reload_clicked(nullptr, bw); return TRUE; }

    return FALSE;
}
