#pragma once

namespace themes {

// ── Retro Windows Vista Aero Glass ────────────────────────────────────────────
static const char* VISTA = R"CSS(
    window {
        background: linear-gradient(180deg, #dce9f5 0%, #c2d8ee 40%, #b8d0e8 100%);
        font-family: "Segoe UI", "Tahoma", "DejaVu Sans", sans-serif;
        font-size: 12px;
    }
    .toolbar-box {
        background: linear-gradient(180deg,
            rgba(180,210,240,0.95) 0%,
            rgba(120,170,220,0.92) 45%,
            rgba(60,120,190,0.98) 46%,
            rgba(80,140,210,0.95) 70%,
            rgba(50,100,170,1.00) 100%);
        border-bottom: 2px solid #1a5fa8;
        padding: 5px 6px;
        box-shadow: inset 0 1px 0 rgba(255,255,255,0.6),
                    0 2px 6px rgba(0,0,0,0.35);
    }
    entry {
        background: linear-gradient(180deg, #f0f6ff 0%, #ffffff 30%, #eaf2ff 100%);
        color: #1a1a2a;
        border-radius: 12px;
        border: 1px solid #6a9fd8;
        padding: 3px 12px;
        font-family: "Segoe UI", sans-serif;
        box-shadow: inset 0 1px 3px rgba(0,0,100,0.15);
    }
    entry:focus { border-color: #3d8bef; }
    button {
        background: linear-gradient(180deg,
            rgba(220,238,255,0.97) 0%,
            rgba(190,218,248,0.95) 45%,
            rgba(130,180,230,0.98) 46%,
            rgba(160,200,240,0.95) 75%,
            rgba(100,155,210,1.00) 100%);
        color: #0a2a50;
        border-radius: 10px;
        border: 1px solid #5a90cc;
        padding: 3px 10px;
        font-family: "Segoe UI", sans-serif;
        font-weight: bold;
        text-shadow: 0 1px 0 rgba(255,255,255,0.7);
        box-shadow: inset 0 1px 0 rgba(255,255,255,0.7);
    }
    button:hover {
        background: linear-gradient(180deg,
            rgba(210,232,255,1.00) 0%,
            rgba(80,150,230,1.00) 50%,
            rgba(60,120,200,1.00) 100%);
        border-color: #3a70bb;
    }
    button:active { background: #3a70bb; }
    .tab-label {
        color: #0a2a50;
        font-size: 11px;
        text-shadow: 0 1px 0 rgba(255,255,255,0.6);
    }
    notebook tab {
        background: linear-gradient(180deg, rgba(190,215,245,0.85), rgba(150,190,230,0.80));
        border-radius: 6px 6px 0 0;
        border: 1px solid #7ab0de;
        padding: 3px 8px;
    }
    notebook tab:checked {
        background: linear-gradient(180deg, rgba(230,245,255,0.98), rgba(200,228,252,0.95));
        border-color: #5a9fd8;
    }
    notebook > header {
        background: linear-gradient(180deg, rgba(160,200,240,0.6), rgba(130,175,225,0.7));
        border-bottom: 2px solid #5a9fd8;
    }
    .statusbar {
        background: linear-gradient(180deg, rgba(190,218,248,0.95), rgba(140,185,230,0.98));
        color: #0a2a50;
        font-size: 11px;
        padding: 3px 10px;
        border-top: 1px solid #5a90cc;
    }
    .proxy-active { color: #005a00; font-weight: bold; }
    .proxy-none { color: #aa0000; font-weight: bold; }
    .tls-ok { color: #005a00; font-weight: bold; }
    .tls-warn { color: #885500; font-weight: bold; }
)CSS";

// ── Modern Dark ────────────────────────────────────────────────────────────────
static const char* MODERN_DARK = R"CSS(
    window {
        background: #0f0f14;
        font-family: "Inter", "Segoe UI", "Cantarell", sans-serif;
        font-size: 12px;
    }
    .toolbar-box {
        background: #16161d;
        padding: 6px 8px;
        border-bottom: 1px solid #24242e;
    }
    entry {
        background: #1c1c24;
        color: #e8e8ea;
        border-radius: 8px;
        border: 1px solid #2a2a36;
        padding: 4px 12px;
        caret-color: #7aa2ff;
    }
    entry:focus {
        border-color: #7aa2ff;
        box-shadow: 0 0 0 2px rgba(122,162,255,0.15);
    }
    button {
        background: #1c1c24;
        color: #d0d0d4;
        border-radius: 8px;
        border: 1px solid #2a2a36;
        padding: 3px 10px;
        font-weight: 500;
    }
    button:hover { background: #24242e; border-color: #353544; color: #ffffff; }
    button:active { background: #0f0f14; }
    .tab-label { color: #c0c0c8; font-size: 11px; }
    notebook tab {
        background: #14141a;
        border-radius: 6px 6px 0 0;
        border: 1px solid #1c1c24;
        border-bottom: none;
        padding: 3px 8px;
        margin-right: 1px;
    }
    notebook tab:checked {
        background: #1c1c24;
        border-color: #2a2a36;
    }
    notebook > header {
        background: #0f0f14;
        border-bottom: 1px solid #2a2a36;
        padding: 3px 4px 0 4px;
    }
    .statusbar {
        background: #16161d;
        color: #808088;
        font-size: 10px;
        padding: 3px 10px;
        border-top: 1px solid #24242e;
    }
    .proxy-active { color: #5cffa0; font-weight: bold; }
    .proxy-none { color: #ff6b6b; font-weight: bold; }
    .tls-ok { color: #5cffa0; }
    .tls-warn { color: #ffb84c; }
)CSS";

// ── Modern Light ───────────────────────────────────────────────────────────────
static const char* MODERN_LIGHT = R"CSS(
    window {
        background: #fafafa;
        font-family: "Inter", "Segoe UI", "Cantarell", sans-serif;
        font-size: 12px;
    }
    .toolbar-box {
        background: #ffffff;
        padding: 6px 8px;
        border-bottom: 1px solid #e4e4e8;
    }
    entry {
        background: #f4f4f6;
        color: #1a1a1f;
        border-radius: 8px;
        border: 1px solid #dcdce0;
        padding: 4px 12px;
        caret-color: #3d6bd3;
    }
    entry:focus {
        border-color: #3d6bd3;
        box-shadow: 0 0 0 2px rgba(61,107,211,0.15);
        background: #ffffff;
    }
    button {
        background: #f4f4f6;
        color: #2a2a35;
        border-radius: 8px;
        border: 1px solid #dcdce0;
        padding: 3px 10px;
        font-weight: 500;
    }
    button:hover { background: #e8e8ec; border-color: #c4c4ca; color: #000000; }
    button:active { background: #dcdce0; }
    .tab-label { color: #3a3a45; font-size: 11px; }
    notebook tab {
        background: #ececf0;
        border-radius: 6px 6px 0 0;
        border: 1px solid #dcdce0;
        border-bottom: none;
        padding: 3px 8px;
        margin-right: 1px;
    }
    notebook tab:checked {
        background: #ffffff;
        border-color: #c8c8cc;
    }
    notebook > header {
        background: #fafafa;
        border-bottom: 1px solid #dcdce0;
        padding: 3px 4px 0 4px;
    }
    .statusbar {
        background: #ffffff;
        color: #6a6a72;
        font-size: 10px;
        padding: 3px 10px;
        border-top: 1px solid #e4e4e8;
    }
    .proxy-active { color: #1a8f3a; font-weight: bold; }
    .proxy-none { color: #c62828; font-weight: bold; }
    .tls-ok { color: #1a8f3a; }
    .tls-warn { color: #b26a00; }
)CSS";

} // namespace themes
