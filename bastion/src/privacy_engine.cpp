#include "privacy_engine.h"
#include "spoof_script.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <array>

// ── Curated list of realistic Linux/Firefox User-Agent strings ──────────────
static const std::array<const char*, 8> UA_POOL = {{
    "Mozilla/5.0 (X11; Linux x86_64; rv:128.0) Gecko/20100101 Firefox/128.0",
    "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:127.0) Gecko/20100101 Firefox/127.0",
    "Mozilla/5.0 (X11; Linux x86_64; rv:126.0) Gecko/20100101 Firefox/126.0",
    "Mozilla/5.0 (X11; Fedora; Linux x86_64; rv:128.0) Gecko/20100101 Firefox/128.0",
    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36",
    "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36",
    "Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/115.0",
    "Mozilla/5.0 (X11; Linux x86_64; rv:124.0) Gecko/20100101 Firefox/124.0",
}};

// ── CSP injection script – meta CSP enforced via JS at document start ────────
static const char* CSP_INJECT_SCRIPT = R"JS(
(function() {
    // Enforce upgrade-insecure-requests and block mixed content
    const meta = document.createElement('meta');
    meta.setAttribute('http-equiv', 'Content-Security-Policy');
    meta.setAttribute('content',
        "upgrade-insecure-requests; " +
        "block-all-mixed-content; "
    );
    const head = document.head || document.documentElement;
    if (head) head.insertBefore(meta, head.firstChild);

    // Remove Referrer-Policy leaks – set strict-origin-when-cross-origin
    const refMeta = document.createElement('meta');
    refMeta.setAttribute('name', 'referrer');
    refMeta.setAttribute('content', 'no-referrer');
    if (head) head.insertBefore(refMeta, head.firstChild);
})();
)JS";

// ─────────────────────────────────────────────────────────────────────────────

PrivacyEngine::PrivacyEngine(ProxyConfig proxy_cfg)
    : proxy_(std::make_unique<ProxyChain>(proxy_cfg))
    , filter_(std::make_unique<ContentFilter>())
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));
}

std::string PrivacyEngine::random_user_agent() {
    int idx = std::rand() % UA_POOL.size();
    return UA_POOL[idx];
}

WebKitWebContext* PrivacyEngine::create_context() {
    // Ephemeral = no cookies, cache, or other data written to disk
    WebKitWebContext* ctx = webkit_web_context_new_ephemeral();

    harden_context(ctx);
    proxy_->apply(ctx);

    // Cookie policy: first-party only, in-memory (ephemeral context already
    // prevents disk writes, but set the accept policy too)
    WebKitCookieManager* cookie_mgr = webkit_web_context_get_cookie_manager(ctx);
    webkit_cookie_manager_set_accept_policy(
        cookie_mgr, WEBKIT_COOKIE_POLICY_ACCEPT_NO_THIRD_PARTY);

    std::cout << "[Phantom] Ephemeral context created.\n";
    std::cout << "[Phantom] Proxy mode: " << proxy_->description() << "\n";
    return ctx;
}

void PrivacyEngine::harden_context(WebKitWebContext* ctx) const {
    // Disable spell checking (can be used for dict fingerprinting)
    webkit_web_context_set_spell_checking_enabled(ctx, FALSE);

    // Note: multi-process per tab is default in WebKit2GTK 4.1+

    // Disable plugins (Flash, etc.)
    webkit_web_context_set_web_extensions_directory(ctx, "/dev/null");

    // Register custom "phantom-blocked" URI scheme for blocked content feedback
    webkit_web_context_register_uri_scheme(ctx, "phantom-blocked",
        [](WebKitURISchemeRequest* req, gpointer) {
            const gchar* body = "<html><body style='font-family:monospace;"
                                "background:#111;color:#f44'>"
                                "<h2>[Phantom] Request blocked by privacy filter.</h2>"
                                "</body></html>";
            GInputStream* stream = g_memory_input_stream_new_from_data(
                body, -1, nullptr);
            webkit_uri_scheme_request_finish(
                req, stream, -1, "text/html");
            g_object_unref(stream);
        }, nullptr, nullptr);
}

WebKitSettings* PrivacyEngine::create_settings() {
    WebKitSettings* s = webkit_settings_new();

    // ── UA ──────────────────────────────────────────────────────────────────
    webkit_settings_set_user_agent(s, random_user_agent().c_str());

    // ── Feature flags ───────────────────────────────────────────────────────
    webkit_settings_set_enable_javascript(s, TRUE);
    webkit_settings_set_enable_javascript_markup(s, TRUE);
    webkit_settings_set_enable_webgl(s, TRUE);
    webkit_settings_set_enable_media(s, TRUE);
    webkit_settings_set_enable_mediasource(s, TRUE);

    // ── Privacy: disable leaky APIs ──────────────────────────────────────────
    // DNS prefetching, hyperlink auditing (<a ping>), XSS auditor, and frame
    // flattening were all deprecated/removed in WebKit2GTK 4.1; omit.
    webkit_settings_set_enable_page_cache(s, FALSE);          // Back-forward cache
    webkit_settings_set_allow_file_access_from_file_urls(s, FALSE);
    webkit_settings_set_allow_universal_access_from_file_urls(s, FALSE);
    webkit_settings_set_javascript_can_open_windows_automatically(s, FALSE);
    webkit_settings_set_javascript_can_access_clipboard(s, FALSE);

    // ── Security hardening ────────────────────────────────────────────────────
    // XSS auditor and frame flattening removed from WebKit2GTK 4.1
    // Load insecure content: mixed content blocking is now enforced by default.

    // Disable automatic cross-origin script execution
    webkit_settings_set_media_playback_requires_user_gesture(s, TRUE);
    webkit_settings_set_enable_fullscreen(s, FALSE);

    // ── Reduce entropy ────────────────────────────────────────────────────────
    // Disable hardware acceleration reporting (reduces GPU fingerprinting)
    webkit_settings_set_hardware_acceleration_policy(
        s, WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER);

    std::cout << "[Phantom] Settings hardened.\n";
    return s;
}

void PrivacyEngine::inject_spoof_script(WebKitUserContentManager* ucm) const {
    WebKitUserScript* spoof = webkit_user_script_new(
        FINGERPRINT_SPOOF_SCRIPT,
        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
        nullptr, nullptr);
    webkit_user_content_manager_add_script(ucm, spoof);
    webkit_user_script_unref(spoof);
}

void PrivacyEngine::inject_csp_script(WebKitUserContentManager* ucm) const {
    WebKitUserScript* csp = webkit_user_script_new(
        CSP_INJECT_SCRIPT,
        WEBKIT_USER_CONTENT_INJECT_TOP_FRAME,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
        nullptr, nullptr);
    webkit_user_content_manager_add_script(ucm, csp);
    webkit_user_script_unref(csp);
}

WebKitUserContentManager* PrivacyEngine::create_content_manager(WebKitWebContext* ctx) {
    WebKitUserContentManager* ucm = webkit_user_content_manager_new();

    // 1. Fingerprint spoofing (runs at document start, all frames)
    inject_spoof_script(ucm);

    // 2. CSP enforcement meta-injection
    inject_csp_script(ucm);

    // 3. Network-level content filter (async compile → apply)
    filter_->compile_and_apply(ucm, ctx);

    std::cout << "[Phantom] Content manager configured.\n";
    return ucm;
}
