#pragma once
#include <string>
#include <unordered_set>
#include <webkit2/webkit2.h>

class ContentFilter {
public:
    ContentFilter();

    // Apply content-filter rules to a WebKit web context.
    // Uses WebKit's native ContentRuleList engine (JSON-based content blocker).
    void compile_and_apply(WebKitUserContentManager* ucm, WebKitWebContext* ctx);

    bool is_tracker(const std::string& host) const;

private:
    std::unordered_set<std::string> tracker_domains_;
    void build_tracker_list();
    std::string build_content_blocker_json() const;

    static void on_content_rules_compiled(GObject* src, GAsyncResult* res, gpointer data);
};
