#include "browser_window.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <glib.h>

static const char* DEFAULT_HOME = "https://search.brave.com/";

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────
BrowserWindow::BrowserWindow(GtkApplication* app,
                             std::shared_ptr<PrivacyEngine> engine)
    : engine_(std::move(engine))
{
    // Create shared ephemeral WebKit context (one Tor circuit for all tabs)
    shared_ctx_ = engine_->create_context();

    build_ui(app);
    open_tab(DEFAULT_HOME);
    gtk_widget_show_all(window_);
}

BrowserWindow::~BrowserWindow() {
    if (shared_ctx_) g_object_unref(shared_ctx_);
}

// ─────────────────────────────────────────────────────────────────────────────
//  UI construction
// ─────────────────────────────────────────────────────────────────────────────
void BrowserWindow::build_ui(GtkApplication* app) {
    window_ = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window_), "Phantom Browser");
    gtk_window_set_default_size(GTK_WINDOW(window_), 1280, 900);

    // ── Windows Vista Aero Glass theme ────────────────────────────────────────
    GtkCssProvider* css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css, R"CSS(

        /* ── Window base: Vista pearl-grey background ── */
        window {
            background: linear-gradient(180deg, #dce9f5 0%, #c2d8ee 40%, #b8d0e8 100%);
            font-family: "Segoe UI", "Tahoma", "DejaVu Sans", sans-serif;
            font-size: 12px;
        }

        /* ── Aero Glass toolbar: deep blue gradient with top shine ── */
        .toolbar-box {
            background: linear-gradient(180deg,
                rgba(180, 210, 240, 0.95) 0%,
                rgba(120, 170, 220, 0.92) 45%,
                rgba(60,  120, 190, 0.98) 46%,
                rgba(80,  140, 210, 0.95) 70%,
                rgba(50,  100, 170, 1.00) 100%
            );
            border-bottom: 2px solid #1a5fa8;
            padding: 5px 6px;
            box-shadow: inset 0 1px 0 rgba(255,255,255,0.6),
                        0 2px 6px rgba(0,0,0,0.35);
        }

        /* ── Address bar: Vista white glass input ── */
        entry {
            background: linear-gradient(180deg,
                #f0f6ff 0%,
                #ffffff 30%,
                #eaf2ff 100%
            );
            color: #1a1a2a;
            border-radius: 12px;
            border: 1px solid #6a9fd8;
            padding: 3px 12px;
            font-family: "Segoe UI", "Tahoma", sans-serif;
            font-size: 12px;
            box-shadow: inset 0 1px 3px rgba(0,0,100,0.15),
                        0 1px 0 rgba(255,255,255,0.5);
        }
        entry:focus {
            border-color: #3d8bef;
            box-shadow: inset 0 1px 3px rgba(0,80,200,0.2),
                        0 0 0 3px rgba(100,170,255,0.3);
        }

        /* ── Aero glass buttons: glossy pill shape ── */
        button {
            background: linear-gradient(180deg,
                rgba(220, 238, 255, 0.97) 0%,
                rgba(190, 218, 248, 0.95) 45%,
                rgba(130, 180, 230, 0.98) 46%,
                rgba(160, 200, 240, 0.95) 75%,
                rgba(100, 155, 210, 1.00) 100%
            );
            color: #0a2a50;
            border-radius: 10px;
            border: 1px solid #5a90cc;
            padding: 3px 10px;
            font-family: "Segoe UI", "Tahoma", sans-serif;
            font-size: 12px;
            font-weight: bold;
            text-shadow: 0 1px 0 rgba(255,255,255,0.7);
            box-shadow: inset 0 1px 0 rgba(255,255,255,0.7),
                        0 2px 4px rgba(0,0,0,0.25);
        }
        button:hover {
            background: linear-gradient(180deg,
                rgba(210, 232, 255, 1.00) 0%,
                rgba(170, 210, 250, 0.98) 45%,
                rgba(80,  150, 230, 1.00) 46%,
                rgba(120, 180, 240, 0.98) 75%,
                rgba(60,  120, 200, 1.00) 100%
            );
            border-color: #3a70bb;
            box-shadow: inset 0 1px 0 rgba(255,255,255,0.8),
                        0 2px 6px rgba(0,60,160,0.35);
        }
        button:active {
            background: linear-gradient(180deg,
                rgba(80,  140, 210, 1.00) 0%,
                rgba(120, 175, 235, 0.98) 50%,
                rgba(180, 215, 250, 1.00) 100%
            );
            box-shadow: inset 0 2px 4px rgba(0,0,0,0.3);
        }

        /* ── Tab labels ── */
        .tab-label {
            color: #0a2a50;
            font-size: 11px;
            font-family: "Segoe UI", "Tahoma", sans-serif;
            text-shadow: 0 1px 0 rgba(255,255,255,0.6);
        }

        /* ── Tabs: Aero glass inactive / active ── */
        notebook tab {
            background: linear-gradient(180deg,
                rgba(190, 215, 245, 0.85) 0%,
                rgba(150, 190, 230, 0.80) 100%
            );
            border-radius: 6px 6px 0 0;
            border: 1px solid #7ab0de;
            border-bottom: none;
            padding: 3px 8px;
            margin-right: 2px;
            box-shadow: inset 0 1px 0 rgba(255,255,255,0.5);
        }
        notebook tab:checked {
            background: linear-gradient(180deg,
                rgba(230, 245, 255, 0.98) 0%,
                rgba(200, 228, 252, 0.95) 100%
            );
            border-color: #5a9fd8;
            box-shadow: inset 0 1px 0 rgba(255,255,255,0.9),
                        0 -1px 4px rgba(100,160,230,0.3);
        }
        notebook > header {
            background: linear-gradient(180deg,
                rgba(160, 200, 240, 0.6) 0%,
                rgba(130, 175, 225, 0.7) 100%
            );
            border-bottom: 2px solid #5a9fd8;
            padding: 3px 4px 0 4px;
        }

        /* ── Status bar: Vista glass footer ── */
        .statusbar {
            background: linear-gradient(180deg,
                rgba(190, 218, 248, 0.95) 0%,
                rgba(140, 185, 230, 0.98) 100%
            );
            color: #0a2a50;
            font-family: "Segoe UI", "Tahoma", sans-serif;
            font-size: 11px;
            padding: 3px 10px;
            border-top: 1px solid #5a90cc;
            box-shadow: inset 0 1px 0 rgba(255,255,255,0.5);
        }

        /* ── Status indicators ── */
        .proxy-active {
            color: #005a00;
            font-weight: bold;
            text-shadow: 0 1px 0 rgba(255,255,255,0.6);
        }
        .proxy-none {
            color: #aa0000;
            font-weight: bold;
        }
        .tls-ok {
            color: #005a00;
            font-weight: bold;
        }
        .tls-warn {
            color: #885500;
            font-weight: bold;
        }

    )CSS", -1, nullptr);

    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    main_box_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window_), main_box_);

    build_toolbar();

    // ── Notebook (tabs) ────────────────────────────────────────────────────
    notebook_ = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook_), TRUE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook_), FALSE);
    gtk_widget_set_vexpand(notebook_, TRUE);
    gtk_box_pack_start(GTK_BOX(main_box_), notebook_, TRUE, TRUE, 0);

    g_signal_connect(notebook_, "switch-page",
                     G_CALLBACK(on_switch_page), this);

    build_statusbar();
}

void BrowserWindow::build_toolbar() {
    toolbar_ = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkStyleContext* sc = gtk_widget_get_style_context(toolbar_);
    gtk_style_context_add_class(sc, "toolbar-box");
    gtk_box_pack_start(GTK_BOX(main_box_), toolbar_, FALSE, FALSE, 0);

    // Navigation buttons
    btn_back_    = gtk_button_new_with_label("◀");
    btn_forward_ = gtk_button_new_with_label("▶");
    btn_reload_  = gtk_button_new_with_label("↺");
    btn_new_tab_ = gtk_button_new_with_label("+");

    gtk_widget_set_tooltip_text(btn_back_,    "Back");
    gtk_widget_set_tooltip_text(btn_forward_, "Forward");
    gtk_widget_set_tooltip_text(btn_reload_,  "Reload");
    gtk_widget_set_tooltip_text(btn_new_tab_, "New Tab");

    g_signal_connect(btn_back_,    "clicked", G_CALLBACK(on_back_clicked),    this);
    g_signal_connect(btn_forward_, "clicked", G_CALLBACK(on_forward_clicked), this);
    g_signal_connect(btn_reload_,  "clicked", G_CALLBACK(on_reload_clicked),  this);
    g_signal_connect(btn_new_tab_, "clicked", G_CALLBACK(on_new_tab_clicked), this);

    gtk_box_pack_start(GTK_BOX(toolbar_), btn_back_,    FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_), btn_forward_, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_), btn_reload_,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar_), btn_new_tab_, FALSE, FALSE, 0);

    // Address bar
    address_entry_ = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(address_entry_), "Enter URL or search...");
    gtk_widget_set_hexpand(address_entry_, TRUE);
    g_signal_connect(address_entry_, "activate",
                     G_CALLBACK(on_address_activate), this);
    gtk_box_pack_start(GTK_BOX(toolbar_), address_entry_, TRUE, TRUE, 4);

    btn_go_ = gtk_button_new_with_label("Go");
    g_signal_connect(btn_go_, "clicked", G_CALLBACK(on_go_clicked), this);
    gtk_box_pack_start(GTK_BOX(toolbar_), btn_go_, FALSE, FALSE, 0);
}

void BrowserWindow::build_statusbar() {
    GtkWidget* bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkStyleContext* sc = gtk_widget_get_style_context(bar);
    gtk_style_context_add_class(sc, "statusbar");
    gtk_box_pack_end(GTK_BOX(main_box_), bar, FALSE, FALSE, 0);

    // Proxy status
    proxy_label_ = gtk_label_new("");
    std::string pdesc = "[🧅 " + engine_->proxy().description() + "]";
    gtk_label_set_text(GTK_LABEL(proxy_label_), pdesc.c_str());
    GtkStyleContext* psc = gtk_widget_get_style_context(proxy_label_);
    gtk_style_context_add_class(psc,
        engine_->proxy().is_active() ? "proxy-active" : "proxy-none");
    gtk_box_pack_start(GTK_BOX(bar), proxy_label_, FALSE, FALSE, 0);

    // Separator
    gtk_box_pack_start(GTK_BOX(bar), gtk_label_new("│"), FALSE, FALSE, 0);

    // TLS status
    tls_label_ = gtk_label_new("[🔒 Waiting...]");
    gtk_box_pack_start(GTK_BOX(bar), tls_label_, FALSE, FALSE, 0);

    // Spacer + memory label
    GtkWidget* spacer = gtk_label_new("");
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_pack_start(GTK_BOX(bar), spacer, TRUE, TRUE, 0);

    gtk_box_pack_end(GTK_BOX(bar), gtk_label_new("[Ephemeral · No disk writes]"),
                     FALSE, FALSE, 4);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tab management
// ─────────────────────────────────────────────────────────────────────────────
Tab BrowserWindow::create_tab(const std::string& uri) {
    Tab tab{};

    // Each tab gets its own content manager (isolated user scripts + filters)
    WebKitUserContentManager* ucm =
        engine_->create_content_manager(shared_ctx_);

    WebKitSettings* settings = engine_->create_settings();

    // Build WebView with shared context + per-tab content manager
    tab.webview = WEBKIT_WEB_VIEW(g_object_new(
        WEBKIT_TYPE_WEB_VIEW,
        "web-context", shared_ctx_,
        "user-content-manager", ucm,
        "settings", settings,
        nullptr));

    g_object_unref(ucm);
    g_object_unref(settings);

    // WebKit signals
    g_signal_connect(tab.webview, "load-changed",
                     G_CALLBACK(on_load_changed), this);
    g_signal_connect(tab.webview, "notify::title",
                     G_CALLBACK(on_notify_title), this);
    g_signal_connect(tab.webview, "notify::uri",
                     G_CALLBACK(on_notify_uri), this);
    g_signal_connect(tab.webview, "decide-policy",
                     G_CALLBACK(on_decide_policy), this);
    g_signal_connect(tab.webview, "load-failed-with-tls-errors",
                     G_CALLBACK(on_tls_error), this);
    g_signal_connect(tab.webview, "notify::estimated-load-progress",
                     G_CALLBACK(on_notify_estimated_progress), this);

    // ── Tab label (title + close button) ─────────────────────────────────────
    tab.label_box  = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    tab.label_text = gtk_label_new("New Tab");
    gtk_label_set_max_width_chars(GTK_LABEL(tab.label_text), 20);
    gtk_label_set_ellipsize(GTK_LABEL(tab.label_text), PANGO_ELLIPSIZE_END);
    GtkStyleContext* lsc = gtk_widget_get_style_context(tab.label_text);
    gtk_style_context_add_class(lsc, "tab-label");

    GtkWidget* close_btn = gtk_button_new_with_label("✕");
    gtk_button_set_relief(GTK_BUTTON(close_btn), GTK_RELIEF_NONE);
    gtk_widget_set_focus_on_click(close_btn, FALSE);

    g_object_set_data(G_OBJECT(close_btn), "webview", tab.webview);
    g_signal_connect(close_btn, "clicked",
                     G_CALLBACK(on_close_tab_clicked), this);

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
        GTK_NOTEBOOK(notebook_),
        webview_widget,
        tab.label_box);

    gtk_widget_show_all(webview_widget);

    tabs_.push_back(tab);

    webkit_web_view_load_uri(tab.webview, uri.c_str());
    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook_), page_num);
    update_nav_buttons();
}

void BrowserWindow::close_tab(int page_num) {
    if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook_)) <= 1) {
        // Don't close the last tab — open blank instead
        if (!tabs_.empty()) {
            webkit_web_view_load_uri(tabs_[0].webview, "about:blank");
        }
        return;
    }

    GtkWidget* page = gtk_notebook_get_nth_page(
        GTK_NOTEBOOK(notebook_), page_num);

    // Find and remove tab from vector
    for (auto it = tabs_.begin(); it != tabs_.end(); ++it) {
        if (GTK_WIDGET(it->webview) == page) {
            tabs_.erase(it);
            break;
        }
    }

    gtk_notebook_remove_page(GTK_NOTEBOOK(notebook_), page_num);
}

Tab* BrowserWindow::current_tab() {
    int page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook_));
    if (page < 0) return nullptr;
    GtkWidget* wv = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook_), page);
    for (auto& t : tabs_)
        if (GTK_WIDGET(t.webview) == wv) return &t;
    return nullptr;
}

int BrowserWindow::tab_index_of(WebKitWebView* wv) {
    for (int i = 0; i < gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook_)); i++) {
        GtkWidget* pg = gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook_), i);
        if (pg == GTK_WIDGET(wv)) return i;
    }
    return -1;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Navigation
// ─────────────────────────────────────────────────────────────────────────────
std::string BrowserWindow::sanitize_uri(const std::string& input) const {
    if (input.empty()) return DEFAULT_HOME;

    // Already a full URI?
    if (input.rfind("http://", 0) == 0 ||
        input.rfind("https://", 0) == 0 ||
        input.rfind("about:", 0) == 0 ||
        input.rfind("phantom-blocked:", 0) == 0)
        return input;

    // Force HTTPS for bare domain-like strings
    if (input.find('.') != std::string::npos &&
        input.find(' ') == std::string::npos) {
        return "https://" + input;
    }

    // Otherwise treat as search query (use privacy-respecting search)
    std::string encoded;
    for (char c : input) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            encoded += c;
        else {
            char buf[4];
            std::snprintf(buf, sizeof(buf), "%%%02X", static_cast<unsigned char>(c));
            encoded += buf;
        }
    }
    return "https://search.brave.com/search?q=" + encoded;
}

void BrowserWindow::navigate(const std::string& input) {
    Tab* tab = current_tab();
    if (!tab) return;
    std::string uri = sanitize_uri(input);
    tab->uri = uri;
    webkit_web_view_load_uri(tab->webview, uri.c_str());
}

// ─────────────────────────────────────────────────────────────────────────────
//  UI update helpers
// ─────────────────────────────────────────────────────────────────────────────
void BrowserWindow::update_nav_buttons() {
    Tab* tab = current_tab();
    if (!tab) return;
    gtk_widget_set_sensitive(btn_back_,
        webkit_web_view_can_go_back(tab->webview));
    gtk_widget_set_sensitive(btn_forward_,
        webkit_web_view_can_go_forward(tab->webview));
}

void BrowserWindow::update_address_bar(const std::string& uri) {
    gtk_entry_set_text(GTK_ENTRY(address_entry_), uri.c_str());
}

void BrowserWindow::update_tls_status(WebKitWebView* wv) {
    GTlsCertificate* cert = nullptr;
    GTlsCertificateFlags flags;
    bool has_tls = webkit_web_view_get_tls_info(wv, &cert, &flags);

    std::string status;
    if (!has_tls) {
        status = "[⚠ No TLS]";
        GtkStyleContext* sc = gtk_widget_get_style_context(tls_label_);
        gtk_style_context_remove_class(sc, "tls-ok");
        gtk_style_context_add_class(sc, "tls-warn");
    } else if (flags != 0) {
        status = "[⚠ TLS Error]";
        GtkStyleContext* sc = gtk_widget_get_style_context(tls_label_);
        gtk_style_context_remove_class(sc, "tls-ok");
        gtk_style_context_add_class(sc, "tls-warn");
    } else {
        status = "[🔒 TLS OK]";
        GtkStyleContext* sc = gtk_widget_get_style_context(tls_label_);
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
            // Also update window title for the active tab
            Tab* cur = current_tab();
            if (cur && cur->webview == wv)
                gtk_window_set_title(GTK_WINDOW(window_),
                    (display + " – Phantom").c_str());
            return;
        }
    }
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
    Tab* tab = bw->current_tab();
    if (tab) webkit_web_view_go_back(tab->webview);
}

void BrowserWindow::on_forward_clicked(GtkWidget*, gpointer self) {
    auto* bw = static_cast<BrowserWindow*>(self);
    Tab* tab = bw->current_tab();
    if (tab) webkit_web_view_go_forward(tab->webview);
}

void BrowserWindow::on_reload_clicked(GtkWidget*, gpointer self) {
    auto* bw = static_cast<BrowserWindow*>(self);
    Tab* tab = bw->current_tab();
    if (tab) webkit_web_view_reload(tab->webview);
}

void BrowserWindow::on_new_tab_clicked(GtkWidget*, gpointer self) {
    auto* bw = static_cast<BrowserWindow*>(self);
    bw->open_tab(DEFAULT_HOME);
}

void BrowserWindow::on_switch_page(GtkNotebook*, GtkWidget* page,
                                    guint /*page_num*/, gpointer self)
{
    auto* bw = static_cast<BrowserWindow*>(self);
    // Find the WebView for this page
    for (auto& tab : bw->tabs_) {
        if (GTK_WIDGET(tab.webview) == page) {
            const gchar* uri = webkit_web_view_get_uri(tab.webview);
            bw->update_address_bar(uri ? uri : "");
            bw->update_nav_buttons();
            bw->update_tls_status(tab.webview);
            const gchar* title = webkit_web_view_get_title(tab.webview);
            if (title) {
                std::string t(title);
                gtk_window_set_title(GTK_WINDOW(bw->window_),
                    (t + " – Phantom").c_str());
            }
            return;
        }
    }
}

void BrowserWindow::on_close_tab_clicked(GtkWidget* btn, gpointer self) {
    auto* bw = static_cast<BrowserWindow*>(self);
    WebKitWebView* wv = WEBKIT_WEB_VIEW(
        g_object_get_data(G_OBJECT(btn), "webview"));
    int idx = bw->tab_index_of(wv);
    if (idx >= 0) bw->close_tab(idx);
}

// ─────────────────────────────────────────────────────────────────────────────
//  WebKit callbacks
// ─────────────────────────────────────────────────────────────────────────────
void BrowserWindow::on_load_changed(WebKitWebView* wv,
                                     WebKitLoadEvent event, gpointer self)
{
    auto* bw = static_cast<BrowserWindow*>(self);
    Tab* cur = bw->current_tab();
    bool is_current = cur && cur->webview == wv;

    switch (event) {
    case WEBKIT_LOAD_STARTED:
        if (is_current)
            gtk_label_set_text(GTK_LABEL(bw->tls_label_), "[🔄 Loading...]");
        break;

    case WEBKIT_LOAD_COMMITTED:
        if (is_current) {
            const gchar* uri = webkit_web_view_get_uri(wv);
            bw->update_address_bar(uri ? uri : "");
            bw->update_tls_status(wv);
        }
        break;

    case WEBKIT_LOAD_FINISHED:
        if (is_current) {
            bw->update_nav_buttons();
            bw->update_tls_status(wv);
        }
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
    }
}

gboolean BrowserWindow::on_decide_policy(WebKitWebView* wv,
                                          WebKitPolicyDecision* decision,
                                          WebKitPolicyDecisionType type,
                                          gpointer self)
{
    auto* bw = static_cast<BrowserWindow*>(self);

    if (type == WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION) {
        WebKitNavigationPolicyDecision* nav =
            WEBKIT_NAVIGATION_POLICY_DECISION(decision);
        WebKitNavigationAction* action =
            webkit_navigation_policy_decision_get_navigation_action(nav);
        WebKitURIRequest* req =
            webkit_navigation_action_get_request(action);
        const gchar* uri = webkit_uri_request_get_uri(req);

        if (uri) {
            std::string u(uri);
            // Block plaintext HTTP (force HTTPS only)
            if (u.rfind("http://", 0) == 0) {
                std::string secure = "https://" + u.substr(7);
                std::cout << "[Phantom] Upgrading HTTP → HTTPS: " << secure << "\n";
                webkit_web_view_load_uri(wv, secure.c_str());
                webkit_policy_decision_ignore(decision);
                return TRUE;
            }
        }
    }

    if (type == WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION) {
        // Open in a new tab instead of a new window
        WebKitNavigationPolicyDecision* nav =
            WEBKIT_NAVIGATION_POLICY_DECISION(decision);
        WebKitNavigationAction* action =
            webkit_navigation_policy_decision_get_navigation_action(nav);
        WebKitURIRequest* req =
            webkit_navigation_action_get_request(action);
        const gchar* uri = webkit_uri_request_get_uri(req);
        if (uri) bw->open_tab(uri);
        webkit_policy_decision_ignore(decision);
        return TRUE;
    }

    webkit_policy_decision_use(decision);
    return TRUE;
}

gboolean BrowserWindow::on_tls_error(WebKitWebView* wv, const gchar* failing_uri,
                                      GTlsCertificate*, GTlsCertificateFlags flags,
                                      gpointer self)
{
    auto* bw = static_cast<BrowserWindow*>(self);
    std::cerr << "[Phantom] TLS error (" << flags << ") for: " << failing_uri << "\n";

    // Show error page instead of proceeding with bad cert
    const char* page =
        "<html><head><style>"
        "body{background:#1a1a2e;color:#ff4444;font-family:monospace;"
        "display:flex;align-items:center;justify-content:center;height:100vh;margin:0}"
        "div{text-align:center;max-width:600px}"
        "h1{color:#ff4444;font-size:2em}"
        "p{color:#aaa}"
        "code{background:#0f3460;padding:4px 8px;border-radius:4px}"
        "</style></head><body><div>"
        "<h1>🔴 TLS Certificate Error</h1>"
        "<p>Phantom refused to connect because the server's TLS certificate is invalid.</p>"
        "<p>This could mean someone is intercepting your connection (MITM attack).</p>"
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

void BrowserWindow::on_notify_estimated_progress(WebKitWebView* wv,
                                                   GParamSpec*, gpointer self)
{
    auto* bw = static_cast<BrowserWindow*>(self);
    Tab* cur = bw->current_tab();
    if (!cur || cur->webview != wv) return;

    double progress = webkit_web_view_get_estimated_load_progress(wv);
    if (progress < 1.0) {
        std::ostringstream s;
        s << "[🔄 " << static_cast<int>(progress * 100) << "%]";
        gtk_label_set_text(GTK_LABEL(bw->tls_label_), s.str().c_str());
    }
    // Final status set by on_load_changed FINISHED
}
