#include "content_filter.h"
#include <sstream>
#include <iostream>

ContentFilter::ContentFilter() {
    build_tracker_list();
}

void ContentFilter::build_tracker_list() {
    // Comprehensive tracker/ad domain list
    tracker_domains_ = {
        // Google tracking
        "google-analytics.com", "googletagmanager.com", "googletagservices.com",
        "doubleclick.net", "googlesyndication.com", "googleadservices.com",
        "googleapis.com", "adservice.google.com", "ssl.google-analytics.com",
        "stats.g.doubleclick.net", "pagead2.googlesyndication.com",
        // Facebook tracking
        "facebook.com", "connect.facebook.net", "graph.facebook.com",
        "pixel.facebook.com", "an.facebook.com", "www.facebook.com",
        // Twitter/X tracking
        "ads-twitter.com", "analytics.twitter.com", "static.ads-twitter.com",
        "syndication.twitter.com", "platform.twitter.com",
        // Amazon tracking
        "amazon-adsystem.com", "assoc-amazon.com", "fls-na.amazon.com",
        "ir-na.amazon-adsystem.com", "s.amazon-adsystem.com",
        // Microsoft tracking
        "bing.com", "bat.bing.com", "c.msn.com", "clarity.ms",
        "ads.microsoft.com", "adnxs.com",
        // Major ad networks
        "outbrain.com", "taboola.com", "zemanta.com", "revcontent.com",
        "criteo.com", "criteo.net", "adroll.com", "bidswitch.net",
        "rubiconproject.com", "pubmatic.com", "openx.net", "openx.com",
        "appnexus.com", "quantserve.com", "scorecardresearch.com",
        "mediaplex.com", "serving-sys.com", "mathtag.com",
        "addthis.com", "addthisedge.com",
        // Analytics
        "hotjar.com", "mixpanel.com", "segment.com", "segment.io",
        "amplitude.com", "fullstory.com", "logrocket.com", "pendo.io",
        "heap.io", "kissmetrics.com", "optimizely.com", "vwo.com",
        "mouseflow.com", "crazyegg.com", "clicktale.net",
        "newrelic.com", "nr-data.net", "nr-assets.net",
        // Data brokers
        "krux.com", "krxd.net", "bluekai.com", "demdex.net",
        "exelator.com", "lotame.com", "adsrvr.org", "casalemedia.com",
        "turn.com", "evidon.com", "nuggad.net", "nexac.com",
        // CDN-based trackers
        "cloudfront.net", // Not all, but commonly abused
        "tracking.g.doubleclick.net",
        // Social widgets that track
        "platform.linkedin.com", "snap.licdn.com",
        "assets.pinterest.com", "log.pinterest.com",
        // Fingerprinting services
        "fingerprintjs.com", "fp.io", "fingerprint.com",
        // Other common trackers
        "chartbeat.com", "chartbeat.net",
        "iovation.com", "threatmetrix.com",
        "branch.io", "app.link",
        "adjust.com", "appsflyer.com", "kochava.com",
        "moatads.com", "doubleverify.com", "adsafe.org",
        "sizmek.com", "flashtalking.com",
        "advertising.com", "aolcloud.net",
        "adhese.com", "adhese.net",
        "lijit.com", "sovrn.com",
        "yieldmo.com", "yieldoptimizer.com",
        "contextweb.com", "pulsepoint.com",
        "smartadserver.com", "smart-agent.nl",
        "tradedoubler.com", "affiliatewindow.com",
        "linksynergy.com", "cj.com", "clickbooth.com",
        "adsystem.com", "an.yandex.ru", "mc.yandex.ru",
        "omtrdc.net", "adobedc.net", "demdex.net",
        "iocnt.net", "idio.com", "demandbase.com",
        "everesttech.net", "rfihub.com", "rfihub.net",
        "tvpixel.com", "adgrx.com", "adkernel.com",
        "serving.com", "ad.gt", "adskeeper.co.uk",
        "spotxchange.com", "spotx.tv",
        "conversantmedia.com", "dotomi.com",
        "undertone.com", "yume.com",
    };
}

bool ContentFilter::is_tracker(const std::string& host) const {
    if (tracker_domains_.count(host)) return true;
    // Check parent domains
    auto dot = host.find('.');
    while (dot != std::string::npos) {
        std::string parent = host.substr(dot + 1);
        if (tracker_domains_.count(parent)) return true;
        dot = parent.find('.');
    }
    return false;
}

std::string ContentFilter::build_content_blocker_json() const {
    // Build WebKit Content Blocker JSON rule list
    // Format: https://developer.apple.com/documentation/safariservices/creating_a_content_blocker
    std::ostringstream json;
    json << "[\n";

    bool first = true;
    for (const auto& domain : tracker_domains_) {
        if (!first) json << ",\n";
        first = false;

        // Block tracker loads from these domains
        json << R"(  {
    "trigger": {
      "url-filter": ")" << domain << R"(",
      "url-filter-is-case-sensitive": false,
      "load-type": ["third-party"]
    },
    "action": { "type": "block" }
  })";
    }

    // Extra rules: block fingerprinting scripts
    json << R"(,
  {
    "trigger": {
      "url-filter": "fingerprint",
      "resource-type": ["script"],
      "load-type": ["third-party"]
    },
    "action": { "type": "block" }
  },
  {
    "trigger": {
      "url-filter": "tracking",
      "resource-type": ["script", "image"],
      "load-type": ["third-party"]
    },
    "action": { "type": "block" }
  },
  {
    "trigger": {
      "url-filter": "pixel",
      "resource-type": ["image"],
      "load-type": ["third-party"]
    },
    "action": { "type": "block" }
  })";

    // Block HTTP upgradeable to HTTPS (force HTTPS equivalent via meta rule)
    // Strip tracking query params
    json << R"(,
  {
    "trigger": {
      "url-filter": "(\\?|&)(utm_source|utm_medium|utm_campaign|utm_content|utm_term|fbclid|gclid|msclkid|dclid|zanpid|igshid|mc_eid|_ga|_gl|ref|source|affiliate_id)=[^&]*",
      "url-filter-is-case-sensitive": false
    },
    "action": {
      "type": "redirect",
      "redirect": { "transform": { "query-transform": { "remove-parameters": ["utm_source","utm_medium","utm_campaign","utm_content","utm_term","fbclid","gclid","msclkid","dclid","zanpid","igshid","mc_eid","_ga","_gl","ref","source","affiliate_id"] } } }
    }
  })";

    json << "\n]";
    return json.str();
}

struct RuleCompileCtx {
    WebKitUserContentManager* ucm;
    WebKitWebContext* ctx;
};

void ContentFilter::on_content_rules_compiled(GObject* src, GAsyncResult* res, gpointer data) {
    GError* error = nullptr;
    WebKitUserContentFilter* filter =
        webkit_user_content_filter_store_save_finish(
            WEBKIT_USER_CONTENT_FILTER_STORE(src), res, &error);

    auto* rctx = static_cast<RuleCompileCtx*>(data);

    if (error) {
        g_printerr("[Bastion] Content filter compile error: %s\n", error->message);
        g_error_free(error);
        delete rctx;
        return;
    }

    webkit_user_content_manager_add_filter(rctx->ucm, filter);
    g_object_unref(filter);

    std::cout << "[Bastion] Content filter rules compiled and applied.\n";
    delete rctx;
}

void ContentFilter::compile_and_apply(WebKitUserContentManager* ucm, WebKitWebContext* ctx) {
    // Build the JSON rule list
    std::string json = build_content_blocker_json();

    // Store it in a temporary filter store in /tmp
    const gchar* store_path = "/tmp/bastion-cf-store";
    WebKitUserContentFilterStore* store =
        webkit_user_content_filter_store_new(store_path);

    GBytes* rule_bytes = g_bytes_new(json.c_str(), json.size());

    auto* rctx = new RuleCompileCtx{ucm, ctx};

    webkit_user_content_filter_store_save(
        store, "bastion-blocklist", rule_bytes,
        nullptr,
        on_content_rules_compiled,
        rctx
    );

    g_bytes_unref(rule_bytes);
    g_object_unref(store);
}
