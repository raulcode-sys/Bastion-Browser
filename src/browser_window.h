#pragma once
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <memory>
#include <string>
#include <vector>
#include "privacy_engine.h"
#include "app_state.h"
#include "download_manager.h"

struct Tab {
    GtkWidget*     label_box;
    GtkWidget*     label_text;
    WebKitWebView* webview;
    std::string    uri;
    bool           pinned = false;
};

class BrowserWindow {
public:
    BrowserWindow(GtkApplication* app,
                  std::shared_ptr<PrivacyEngine> engine,
                  std::shared_ptr<AppState>      state);
    ~BrowserWindow();

    GtkWidget* widget() const { return window_; }

private:
    // ── Core ─────────────────────────────────────────────────────────────────
    GtkWidget* window_;
    GtkWidget* main_box_;
    GtkWidget* toolbar_;
    GtkWidget* notebook_;
    GtkWidget* statusbar_;

    // ── Toolbar widgets ──────────────────────────────────────────────────────
    GtkWidget* btn_back_;
    GtkWidget* btn_forward_;
    GtkWidget* btn_reload_;
    GtkWidget* btn_home_;
    GtkWidget* btn_new_tab_;
    GtkWidget* address_entry_;
    GtkWidget* btn_go_;
    GtkWidget* btn_bookmark_;
    GtkWidget* btn_downloads_;
    GtkWidget* btn_reader_;
    GtkWidget* btn_fortress_;
    GtkWidget* btn_menu_;
    GtkWidget* proxy_label_;
    GtkWidget* tls_label_;

    // ── Find bar ─────────────────────────────────────────────────────────────
    GtkWidget* find_bar_;
    GtkWidget* find_entry_;
    bool       find_visible_ = false;

    // ── State ────────────────────────────────────────────────────────────────
    std::shared_ptr<PrivacyEngine> engine_;
    std::shared_ptr<AppState>      state_;
    std::unique_ptr<DownloadManager> downloads_;
    WebKitWebContext*              shared_ctx_;
    GtkCssProvider*                css_provider_ = nullptr;
    std::vector<Tab>               tabs_;

    // ── Setup ────────────────────────────────────────────────────────────────
    void build_ui(GtkApplication* app);
    void build_toolbar();
    void build_statusbar();
    void build_find_bar();

    void apply_theme();
    void register_bastion_scheme();
    void handle_bastion_request(WebKitURISchemeRequest* req);
    void handle_bastion_action (WebKitURISchemeRequest* req, const std::string& query);

    Tab  create_tab(const std::string& uri);
    void open_tab  (const std::string& uri = "bastion://home");
    void close_tab (int page_num);
    Tab* current_tab();
    int  tab_index_of(WebKitWebView* wv);

    void        navigate    (const std::string& input);
    std::string sanitize_uri(const std::string& input) const;

    void update_nav_buttons();
    void update_address_bar (const std::string& uri);
    void update_tls_status  (WebKitWebView* wv);
    void update_tab_title   (WebKitWebView* wv, const std::string& title);
    void update_bookmark_button();
    void update_downloads_button();

    void find_show();
    void find_hide();
    void find_next();
    void find_prev();

    void zoom_in();
    void zoom_out();
    void zoom_reset();

    void toggle_bookmark_current();

    // New features
    void toggle_reader_mode(WebKitWebView* wv);
    void toggle_fortress();
    void request_pip(WebKitWebView* wv);
    void view_source();
    void open_inspector();
    void apply_site_settings(WebKitWebView* wv, const std::string& uri);
    void restore_last_session();
    void save_session();

    // Tab operations
    void duplicate_tab(int page_num);
    void toggle_pin   (int page_num);
    void close_others (int page_num);
    void close_right  (int page_num);
    void close_left   (int page_num);
    void show_tab_menu(GtkWidget* btn, int page_num, GdkEventButton* event);

    // ── Static trampolines ───────────────────────────────────────────────────
    static void on_go_clicked        (GtkWidget*, gpointer);
    static void on_back_clicked      (GtkWidget*, gpointer);
    static void on_forward_clicked   (GtkWidget*, gpointer);
    static void on_reload_clicked    (GtkWidget*, gpointer);
    static void on_home_clicked      (GtkWidget*, gpointer);
    static void on_new_tab_clicked   (GtkWidget*, gpointer);
    static void on_bookmark_clicked  (GtkWidget*, gpointer);
    static void on_downloads_clicked (GtkWidget*, gpointer);
    static void on_reader_clicked    (GtkWidget*, gpointer);
    static void on_fortress_clicked  (GtkWidget*, gpointer);
    static void on_menu_clicked      (GtkWidget*, gpointer);
    static void on_address_activate  (GtkWidget*, gpointer);
    static void on_switch_page       (GtkNotebook*, GtkWidget*, guint, gpointer);
    static void on_close_tab_clicked (GtkWidget*, gpointer);
    static gboolean on_tab_button_press(GtkWidget*, GdkEventButton*, gpointer);

    static void on_find_activate     (GtkWidget*, gpointer);
    static void on_find_next_clicked (GtkWidget*, gpointer);
    static void on_find_prev_clicked (GtkWidget*, gpointer);
    static void on_find_close_clicked(GtkWidget*, gpointer);

    static void     on_load_changed      (WebKitWebView*, WebKitLoadEvent, gpointer);
    static void     on_notify_title      (WebKitWebView*, GParamSpec*, gpointer);
    static void     on_notify_uri        (WebKitWebView*, GParamSpec*, gpointer);
    static gboolean on_decide_policy     (WebKitWebView*, WebKitPolicyDecision*,
                                          WebKitPolicyDecisionType, gpointer);
    static gboolean on_tls_error         (WebKitWebView*, const gchar*, GTlsCertificate*,
                                          GTlsCertificateFlags, gpointer);
    static void     on_notify_estimated_progress(WebKitWebView*, GParamSpec*, gpointer);

    static void     on_bastion_scheme(WebKitURISchemeRequest*, gpointer);
    static gboolean on_key_press     (GtkWidget*, GdkEventKey*, gpointer);
    static gboolean on_context_menu  (WebKitWebView*, WebKitContextMenu*,
                                      GdkEvent*, WebKitHitTestResult*, gpointer);
    static gboolean on_window_delete (GtkWidget*, GdkEvent*, gpointer);
};
