#pragma once
#include <webkit2/webkit2.h>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

enum class DLStatus {
    STARTING,     // destination not yet chosen
    IN_PROGRESS,
    FINISHED,
    FAILED,
    CANCELLED
};

struct DownloadItem {
    WebKitDownload* wk_download = nullptr;  // nullptr when completed/removed
    std::string     id;                     // unique id (millis timestamp + counter)
    std::string     url;
    std::string     filename;               // basename only
    std::string     destination;            // full local path
    guint64         total_bytes    = 0;
    guint64         received_bytes = 0;
    double          progress       = 0.0;   // 0..1
    DLStatus        status         = DLStatus::STARTING;
    std::string     error_message;
    int64_t         started_at     = 0;     // unix seconds
};

// Forward-declare so we can make DownloadManager hold a callback back to browser
class BrowserWindow;

// ─────────────────────────────────────────────────────────────────────────────
//  DownloadManager
//  Watches the WebKitWebContext's "download-started" signal and manages the
//  lifecycle of every active download: destination prompt, progress, cancel,
//  completion.  Keeps an in-session list for display in bastion://downloads.
//
//  Downloads are saved by default to ~/Downloads.  File path is chosen
//  automatically (no chooser dialog) — browser streams straight to disk.
// ─────────────────────────────────────────────────────────────────────────────
class DownloadManager {
public:
    DownloadManager();
    ~DownloadManager();

    // Attach to a web context so we receive download-started signals.
    void attach(WebKitWebContext* ctx);

    // Direct trigger (e.g., right-click "Save link as"):
    void start_from_uri(WebKitWebContext* ctx, const std::string& uri);

    // Controls
    void cancel(const std::string& id);
    void remove(const std::string& id); // remove from list (only if completed/failed/cancelled)
    void clear_completed();

    // Access
    const std::vector<std::shared_ptr<DownloadItem>>& items() const { return items_; }

    // Active count (for toolbar badge)
    int active_count() const;

    // Callback fired any time an item changes. UI can re-render.
    using ChangeCb = void(*)(void* userdata);
    void set_change_callback(ChangeCb cb, void* userdata) {
        change_cb_ = cb; change_ud_ = userdata;
    }

private:
    std::vector<std::shared_ptr<DownloadItem>> items_;
    ChangeCb change_cb_ = nullptr;
    void*    change_ud_ = nullptr;
    int      id_counter_ = 0;

    std::shared_ptr<DownloadItem> find_by_download(WebKitDownload* d);
    std::shared_ptr<DownloadItem> find_by_id(const std::string& id);

    void wire_download_signals(WebKitDownload* d, std::shared_ptr<DownloadItem> item);
    void notify_changed();

    // Utility
    static std::string downloads_dir();
    static std::string unique_destination(const std::string& dir,
                                          const std::string& filename);

    // Static GTK signal trampolines
    static void     on_download_started (WebKitWebContext*, WebKitDownload*, gpointer self);
    static gboolean on_decide_destination(WebKitDownload*, const gchar* suggested_filename, gpointer self);
    static void     on_received_data    (WebKitDownload*, guint64 data_length, gpointer self);
    static void     on_finished         (WebKitDownload*, gpointer self);
    static void     on_failed           (WebKitDownload*, gpointer error, gpointer self);
    static void     on_notify_progress  (WebKitDownload*, GParamSpec*, gpointer self);
};
