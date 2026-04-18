#include "download_manager.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>

namespace fs = std::filesystem;

DownloadManager::DownloadManager() = default;
DownloadManager::~DownloadManager() = default;

// ─────────────────────────────────────────────────────────────────────────────
//  Attach to context
// ─────────────────────────────────────────────────────────────────────────────
void DownloadManager::attach(WebKitWebContext* ctx) {
    g_signal_connect(ctx, "download-started",
                     G_CALLBACK(on_download_started), this);
}

void DownloadManager::start_from_uri(WebKitWebContext* ctx, const std::string& uri) {
    // This triggers download-started which we handle normally
    webkit_web_context_download_uri(ctx, uri.c_str());
}

// ─────────────────────────────────────────────────────────────────────────────
//  Lookup helpers
// ─────────────────────────────────────────────────────────────────────────────
std::shared_ptr<DownloadItem> DownloadManager::find_by_download(WebKitDownload* d) {
    for (auto& it : items_)
        if (it->wk_download == d) return it;
    return nullptr;
}

std::shared_ptr<DownloadItem> DownloadManager::find_by_id(const std::string& id) {
    for (auto& it : items_)
        if (it->id == id) return it;
    return nullptr;
}

int DownloadManager::active_count() const {
    int n = 0;
    for (const auto& it : items_)
        if (it->status == DLStatus::IN_PROGRESS || it->status == DLStatus::STARTING) n++;
    return n;
}

void DownloadManager::notify_changed() {
    if (change_cb_) change_cb_(change_ud_);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Controls
// ─────────────────────────────────────────────────────────────────────────────
void DownloadManager::cancel(const std::string& id) {
    auto it = find_by_id(id);
    if (!it) return;
    if (it->wk_download &&
        (it->status == DLStatus::IN_PROGRESS || it->status == DLStatus::STARTING))
    {
        webkit_download_cancel(it->wk_download);
        // The "failed" signal will fire with a CANCELLED error
    }
}

void DownloadManager::remove(const std::string& id) {
    items_.erase(std::remove_if(items_.begin(), items_.end(),
        [&](const std::shared_ptr<DownloadItem>& it){
            if (it->id != id) return false;
            // Only remove if not active
            return it->status != DLStatus::IN_PROGRESS &&
                   it->status != DLStatus::STARTING;
        }), items_.end());
    notify_changed();
}

void DownloadManager::clear_completed() {
    items_.erase(std::remove_if(items_.begin(), items_.end(),
        [](const std::shared_ptr<DownloadItem>& it){
            return it->status == DLStatus::FINISHED ||
                   it->status == DLStatus::FAILED   ||
                   it->status == DLStatus::CANCELLED;
        }), items_.end());
    notify_changed();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Destination path
// ─────────────────────────────────────────────────────────────────────────────
std::string DownloadManager::downloads_dir() {
    const char* home = std::getenv("HOME");
    std::string dir = (home ? std::string(home) : "/tmp") + "/Downloads";
    try { fs::create_directories(dir); } catch (...) {}
    return dir;
}

std::string DownloadManager::unique_destination(const std::string& dir,
                                                const std::string& filename)
{
    std::string base = filename.empty() ? "download" : filename;

    // Strip any path separators just in case
    auto last_slash = base.find_last_of("/\\");
    if (last_slash != std::string::npos) base = base.substr(last_slash + 1);
    if (base.empty()) base = "download";

    std::string path = dir + "/" + base;
    if (!fs::exists(path)) return path;

    // Split into stem + extension
    std::string stem = base, ext;
    auto dot = base.find_last_of('.');
    if (dot != std::string::npos && dot != 0) {
        stem = base.substr(0, dot);
        ext  = base.substr(dot);
    }
    for (int i = 1; i < 9999; i++) {
        std::ostringstream o;
        o << dir << "/" << stem << " (" << i << ")" << ext;
        if (!fs::exists(o.str())) return o.str();
    }
    // Extreme fallback
    return dir + "/" + base + ".dup";
}

// ─────────────────────────────────────────────────────────────────────────────
//  Signal wiring
// ─────────────────────────────────────────────────────────────────────────────
void DownloadManager::wire_download_signals(WebKitDownload* d,
                                             std::shared_ptr<DownloadItem>)
{
    g_signal_connect(d, "decide-destination",
                     G_CALLBACK(on_decide_destination), this);
    g_signal_connect(d, "received-data",
                     G_CALLBACK(on_received_data),      this);
    g_signal_connect(d, "finished",
                     G_CALLBACK(on_finished),           this);
    g_signal_connect(d, "failed",
                     G_CALLBACK(on_failed),             this);
    g_signal_connect(d, "notify::estimated-progress",
                     G_CALLBACK(on_notify_progress),    this);
}

// ─────────────────────────────────────────────────────────────────────────────
//  download-started — the entry point for every download
// ─────────────────────────────────────────────────────────────────────────────
void DownloadManager::on_download_started(WebKitWebContext*,
                                           WebKitDownload* download,
                                           gpointer self_ptr)
{
    auto* self = static_cast<DownloadManager*>(self_ptr);

    auto item = std::make_shared<DownloadItem>();
    item->wk_download = download;
    item->started_at  = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // Unique ID
    std::ostringstream oid;
    oid << item->started_at << "_" << (++self->id_counter_);
    item->id = oid.str();

    // Source URL
    WebKitURIRequest* req = webkit_download_get_request(download);
    if (req) {
        const gchar* u = webkit_uri_request_get_uri(req);
        if (u) item->url = u;
    }

    item->status = DLStatus::STARTING;
    self->items_.push_back(item);

    self->wire_download_signals(download, item);

    std::cout << "[Bastion] Download started: " << item->url << "\n";
    self->notify_changed();
}

// ─────────────────────────────────────────────────────────────────────────────
//  decide-destination — returning TRUE means "we handled it"
// ─────────────────────────────────────────────────────────────────────────────
gboolean DownloadManager::on_decide_destination(WebKitDownload* d,
                                                 const gchar* suggested_filename,
                                                 gpointer self_ptr)
{
    auto* self = static_cast<DownloadManager*>(self_ptr);
    auto item = self->find_by_download(d);
    if (!item) return FALSE;

    std::string name = suggested_filename ? suggested_filename : "download";
    std::string dir  = downloads_dir();
    std::string dest = unique_destination(dir, name);

    item->filename    = dest.substr(dest.find_last_of('/') + 1);
    item->destination = dest;
    item->status      = DLStatus::IN_PROGRESS;

    // Convert to file:// URI for WebKit
    std::string uri = "file://" + dest;
    webkit_download_set_allow_overwrite(d, FALSE);
    webkit_download_set_destination(d, uri.c_str());

    // Grab total size from response if available
    WebKitURIResponse* resp = webkit_download_get_response(d);
    if (resp) item->total_bytes = webkit_uri_response_get_content_length(resp);

    std::cout << "[Bastion] Saving to: " << dest << "\n";
    self->notify_changed();
    return TRUE;
}

// ─────────────────────────────────────────────────────────────────────────────
//  received-data — progress updates
// ─────────────────────────────────────────────────────────────────────────────
void DownloadManager::on_received_data(WebKitDownload* d,
                                        guint64 /*data_length*/,
                                        gpointer self_ptr)
{
    auto* self = static_cast<DownloadManager*>(self_ptr);
    auto item = self->find_by_download(d);
    if (!item) return;

    item->received_bytes = webkit_download_get_received_data_length(d);
    item->progress       = webkit_download_get_estimated_progress(d);

    if (item->total_bytes == 0) {
        WebKitURIResponse* resp = webkit_download_get_response(d);
        if (resp) item->total_bytes = webkit_uri_response_get_content_length(resp);
    }
    self->notify_changed();
}

void DownloadManager::on_notify_progress(WebKitDownload* d, GParamSpec*, gpointer self_ptr) {
    auto* self = static_cast<DownloadManager*>(self_ptr);
    auto item = self->find_by_download(d);
    if (!item) return;
    item->progress = webkit_download_get_estimated_progress(d);
    self->notify_changed();
}

// ─────────────────────────────────────────────────────────────────────────────
//  finished
// ─────────────────────────────────────────────────────────────────────────────
void DownloadManager::on_finished(WebKitDownload* d, gpointer self_ptr) {
    auto* self = static_cast<DownloadManager*>(self_ptr);
    auto item = self->find_by_download(d);
    if (!item) return;

    // If it was cancelled, the "failed" signal already set status; don't override
    if (item->status == DLStatus::CANCELLED) {
        item->wk_download = nullptr;
        self->notify_changed();
        return;
    }

    item->status      = DLStatus::FINISHED;
    item->progress    = 1.0;
    item->wk_download = nullptr;

    // Pull final size
    item->received_bytes = webkit_download_get_received_data_length(d);
    if (item->total_bytes == 0) item->total_bytes = item->received_bytes;

    std::cout << "[Bastion] Download finished: " << item->destination << "\n";
    self->notify_changed();
}

// ─────────────────────────────────────────────────────────────────────────────
//  failed (includes user cancellation)
// ─────────────────────────────────────────────────────────────────────────────
void DownloadManager::on_failed(WebKitDownload* d, gpointer err_ptr, gpointer self_ptr) {
    auto* self = static_cast<DownloadManager*>(self_ptr);
    auto item = self->find_by_download(d);
    if (!item) return;

    GError* err = static_cast<GError*>(err_ptr);
    // WEBKIT_DOWNLOAD_ERROR_CANCELLED_BY_USER = 2
    bool cancelled = err && err->domain == WEBKIT_DOWNLOAD_ERROR &&
                     err->code == WEBKIT_DOWNLOAD_ERROR_CANCELLED_BY_USER;

    item->status        = cancelled ? DLStatus::CANCELLED : DLStatus::FAILED;
    item->error_message = (err && err->message) ? err->message : "Unknown error";
    item->wk_download   = nullptr;

    std::cout << "[Bastion] Download " << (cancelled ? "cancelled" : "failed")
              << ": " << item->error_message << "\n";
    self->notify_changed();
}
