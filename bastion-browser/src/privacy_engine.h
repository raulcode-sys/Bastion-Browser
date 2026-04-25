#pragma once
#include <memory>
#include <string>
#include <webkit2/webkit2.h>
#include "proxy_chain.h"
#include "content_filter.h"

// ─────────────────────────────────────────────────────────────────────────────
//  PrivacyEngine  –  Orchestrates all privacy and security measures:
//   • Fingerprint spoofing via injected JS
//   • Content/tracker blocking via WebKit Content Rules
//   • Proxy chain (Tor / I2P / custom SOCKS5)
//   • Cookie isolation (accept first-party only, in-memory)
//   • Security settings hardening
//   • User-Agent rotation
//   • HTTPS enforcement (upgrade-insecure-requests CSP)
// ─────────────────────────────────────────────────────────────────────────────

class PrivacyEngine {
public:
    explicit PrivacyEngine(ProxyConfig proxy_cfg);
    ~PrivacyEngine() = default;

    // Creates and configures an ephemeral WebKitWebContext with all privacy
    // measures applied.  Caller takes ownership (g_object_unref when done).
    WebKitWebContext* create_context();

    // Creates a WebKitSettings object hardened for privacy.
    // Caller takes ownership.
    WebKitSettings* create_settings();

    // Creates a WebKitUserContentManager with fingerprint spoof script
    // and content filters pre-loaded.
    // Caller takes ownership.
    WebKitUserContentManager* create_content_manager(WebKitWebContext* ctx);

    const ProxyChain& proxy() const { return *proxy_; }

    // Rotate to a random but realistic User-Agent string
    static std::string random_user_agent();

private:
    std::unique_ptr<ProxyChain>    proxy_;
    std::unique_ptr<ContentFilter> filter_;

    void harden_context(WebKitWebContext* ctx) const;
    void inject_csp_script(WebKitUserContentManager* ucm) const;
    void inject_spoof_script(WebKitUserContentManager* ucm) const;
};
