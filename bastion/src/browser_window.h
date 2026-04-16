#pragma once
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <memory>
#include <string>
#include <vector>
#include "privacy_engine.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Tab  –  represents a single browser tab with its own isolated WebView.
//  Each tab shares the same ephemeral WebKitWebContext (same Tor circuit) but
//  has a separate WebKitUserContentManager, so user scripts are tab-isolated.
// ─────────────────────────────────────────────────────────────────────────────
struct Tab {
    GtkWidget*   label_box;   // Box inside the tab label (title + close btn)
    GtkWidget*   label_text;  // GtkLabel showing page title
    WebKitWebView* webview;
    std::string  uri;
};

// ─────────────────────────────────────────────────────────────────────────────
//  BrowserWindow  –  GTK3 application window.
//  Provides: tabbed browsing, address bar, navigation controls,
//            proxy/privacy status bar, per-tab WebKit views.
// ─────────────────────────────────────────────────────────────────────────────
class BrowserWindow {
public:
    explicit BrowserWindow(GtkApplication* app, std::shared_ptr<PrivacyEngine> engine);
    ~BrowserWindow();

    GtkWidget* widget() const { return window_; }

private:
    // ── Core ─────────────────────────────────────────────────────────────────
    GtkWidget*   window_;
    GtkWidget*   main_box_;      // Vertical box: toolbar + notebook + statusbar
    GtkWidget*   toolbar_;       // Horizontal box: back, fwd, reload, address, go
    GtkWidget*   notebook_;      // Tab container
    GtkWidget*   statusbar_;     // Proxy / TLS status

    // ── Toolbar widgets ──────────────────────────────────────────────────────
    GtkWidget*   btn_back_;
    GtkWidget*   btn_forward_;
    GtkWidget*   btn_reload_;
    GtkWidget*   btn_new_tab_;
    GtkWidget*   address_entry_;
    GtkWidget*   btn_go_;
    GtkWidget*   proxy_label_;   // Shows current proxy mode
    GtkWidget*   tls_label_;     // Shows TLS status of current page

    // ── State ────────────────────────────────────────────────────────────────
    std::shared_ptr<PrivacyEngine> engine_;
    WebKitWebContext*              shared_ctx_;   // Shared ephemeral context
    std::vector<Tab>               tabs_;

    // ── Setup ────────────────────────────────────────────────────────────────
    void build_ui(GtkApplication* app);
    void build_toolbar();
    void build_statusbar();

    // ── Tab management ────────────────────────────────────────────────────────
    Tab create_tab(const std::string& uri);
    void open_tab(const std::string& uri = "about:blank");
    void close_tab(int page_num);
    Tab* current_tab();
    int  tab_index_of(WebKitWebView* wv);

    // ── Navigation ────────────────────────────────────────────────────────────
    void navigate(const std::string& input);
    std::string sanitize_uri(const std::string& input) const;

    // ── UI updates ────────────────────────────────────────────────────────────
    void update_nav_buttons();
    void update_address_bar(const std::string& uri);
    void update_tls_status(WebKitWebView* wv);
    void update_tab_title(WebKitWebView* wv, const std::string& title);

    // ── GTK signal callbacks (static trampolines) ─────────────────────────────
    static void on_go_clicked       (GtkWidget*, gpointer self);
    static void on_back_clicked     (GtkWidget*, gpointer self);
    static void on_forward_clicked  (GtkWidget*, gpointer self);
    static void on_reload_clicked   (GtkWidget*, gpointer self);
    static void on_new_tab_clicked  (GtkWidget*, gpointer self);
    static void on_address_activate (GtkWidget*, gpointer self);
    static void on_switch_page      (GtkNotebook*, GtkWidget*, guint, gpointer self);
    static void on_close_tab_clicked(GtkWidget* btn, gpointer self);

    // ── WebKit signal callbacks (static trampolines) ──────────────────────────
    static void on_load_changed     (WebKitWebView*, WebKitLoadEvent, gpointer self);
    static void on_notify_title     (WebKitWebView*, GParamSpec*, gpointer self);
    static void on_notify_uri       (WebKitWebView*, GParamSpec*, gpointer self);
    static gboolean on_decide_policy(WebKitWebView*, WebKitPolicyDecision*,
                                     WebKitPolicyDecisionType, gpointer self);
    static gboolean on_tls_error    (WebKitWebView*, const gchar*, GTlsCertificate*,
                                     GTlsCertificateFlags, gpointer self);
    static void on_notify_estimated_progress(WebKitWebView*, GParamSpec*, gpointer self);
};
