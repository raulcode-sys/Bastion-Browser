#pragma once

static const char* FINGERPRINT_SPOOF_SCRIPT = R"JS(
(function() {
    'use strict';

    // ── Canvas API Poisoning ──────────────────────────────────────────────
    const _toDataURL  = HTMLCanvasElement.prototype.toDataURL;
    const _toBlob     = HTMLCanvasElement.prototype.toBlob;
    const _getContext = HTMLCanvasElement.prototype.getContext;

    function noiseImageData(imageData) {
        const d = imageData.data;
        for (let i = 0; i < d.length; i += 4) {
            d[i]   = Math.max(0, Math.min(255, d[i]   + (Math.random() < 0.5 ? -1 : 1)));
            d[i+1] = Math.max(0, Math.min(255, d[i+1] + (Math.random() < 0.5 ? -1 : 1)));
            d[i+2] = Math.max(0, Math.min(255, d[i+2] + (Math.random() < 0.5 ? -1 : 1)));
        }
        return imageData;
    }

    HTMLCanvasElement.prototype.getContext = function(type, opts) {
        const ctx = _getContext.call(this, type, opts);
        if (!ctx) return ctx;
        if (type === '2d') {
            const _getImageData = ctx.getImageData.bind(ctx);
            ctx.getImageData = function(x, y, w, h) {
                return noiseImageData(_getImageData(x, y, w, h));
            };
        }
        return ctx;
    };

    HTMLCanvasElement.prototype.toDataURL = function() {
        const ctx2 = _getContext.call(this, '2d');
        if (ctx2 && this.width > 0 && this.height > 0) {
            const id = ctx2.getImageData(0, 0, this.width || 1, this.height || 1);
            noiseImageData(id);
            ctx2.putImageData(id, 0, 0);
        }
        return _toDataURL.apply(this, arguments);
    };

    // ── WebGL Fingerprint Spoofing ────────────────────────────────────────
    const SPOOF_VENDOR   = 'Intel Inc.';
    const SPOOF_RENDERER = 'Intel Iris Plus Graphics';

    function patchWebGLProto(proto) {
        const _getParam = proto.getParameter;
        proto.getParameter = function(p) {
            if (p === 0x9245) return SPOOF_VENDOR;    // UNMASKED_VENDOR_WEBGL
            if (p === 0x9246) return SPOOF_RENDERER;  // UNMASKED_RENDERER_WEBGL
            if (p === 0x1F00) return SPOOF_VENDOR;
            if (p === 0x1F01) return SPOOF_RENDERER;
            return _getParam.call(this, p);
        };
        const _getExts = proto.getSupportedExtensions;
        proto.getSupportedExtensions = function() {
            const exts = _getExts.call(this) || [];
            return exts.filter(e => e !== 'WEBGL_debug_renderer_info' &&
                                    e !== 'WEBGL_debug_shaders');
        };
        const _readPixels = proto.readPixels;
        proto.readPixels = function(x, y, w, h, fmt, type, buf) {
            _readPixels.apply(this, arguments);
            if (buf instanceof Uint8Array || buf instanceof Uint8ClampedArray) {
                for (let i = 0; i < Math.min(buf.length, 64); i++) {
                    buf[i] = Math.max(0, Math.min(255, buf[i] + (Math.random() < 0.5 ? -1 : 1)));
                }
            }
        };
    }
    if (window.WebGLRenderingContext)  patchWebGLProto(WebGLRenderingContext.prototype);
    if (window.WebGL2RenderingContext) patchWebGLProto(WebGL2RenderingContext.prototype);

    // ── AudioContext Fingerprint Spoofing ─────────────────────────────────
    const AudioCtx = window.AudioContext || window.webkitAudioContext;
    if (AudioCtx) {
        const _createAnalyser = AudioCtx.prototype.createAnalyser;
        AudioCtx.prototype.createAnalyser = function() {
            const node = _createAnalyser.call(this);
            const _getFloat = node.getFloatFrequencyData.bind(node);
            const _getByte  = node.getByteFrequencyData.bind(node);
            node.getFloatFrequencyData = function(arr) {
                _getFloat(arr);
                for (let i = 0; i < arr.length; i++) arr[i] += (Math.random() - 0.5) * 0.0001;
            };
            node.getByteFrequencyData = function(arr) {
                _getByte(arr);
                for (let i = 0; i < arr.length; i++) arr[i] = Math.max(0, Math.min(255, arr[i] + (Math.random() < 0.5 ? -1 : 1)));
            };
            return node;
        };
        // Spoof sampleRate to common value
        Object.defineProperty(AudioCtx.prototype, 'sampleRate', {
            get: function() { return 44100; }
        });
    }

    // ── Navigator Properties ──────────────────────────────────────────────
    const fakePlugins = { length: 0, item: () => null, namedItem: () => null, refresh: () => {},
        [Symbol.iterator]: function*() {} };
    const fakeMimes   = { length: 0, item: () => null, namedItem: () => null,
        [Symbol.iterator]: function*() {} };

    const navProps = {
        hardwareConcurrency: 4,
        deviceMemory: 8,
        maxTouchPoints: 0,
        platform: 'Linux x86_64',
        language: 'en-US',
        languages: Object.freeze(['en-US', 'en']),
        plugins: fakePlugins,
        mimeTypes: fakeMimes,
        doNotTrack: '1',
        webdriver: false,
        vendor: 'Google Inc.',
        vendorSub: '',
        productSub: '20030107',
        appCodeName: 'Mozilla',
        appName: 'Netscape',
        appVersion: '5.0 (X11)',
        oscpu: 'Linux x86_64',
        buildID: undefined,
    };
    for (const [k, v] of Object.entries(navProps)) {
        try { Object.defineProperty(navigator, k, { get: () => v, configurable: true }); } catch(_) {}
    }

    // Battery API spoofing
    if (navigator.getBattery) {
        navigator.getBattery = () => Promise.resolve({
            charging: true, chargingTime: 0, dischargingTime: Infinity, level: 1.0,
            addEventListener: () => {}, removeEventListener: () => {}, dispatchEvent: () => true
        });
    }

    // ── Screen & Window Properties ────────────────────────────────────────
    const screenProps = {
        width: 1920, height: 1080,
        availWidth: 1920, availHeight: 1040,
        colorDepth: 24, pixelDepth: 24,
        availLeft: 0, availTop: 0,
        orientation: { type: 'landscape-primary', angle: 0,
            addEventListener: () => {}, removeEventListener: () => {} }
    };
    for (const [k, v] of Object.entries(screenProps)) {
        try { Object.defineProperty(screen, k, { get: () => v, configurable: true }); } catch(_) {}
    }
    try { Object.defineProperty(window, 'devicePixelRatio', { get: () => 1.0, configurable: true }); } catch(_) {}
    try { Object.defineProperty(window, 'outerWidth',  { get: () => 1920, configurable: true }); } catch(_) {}
    try { Object.defineProperty(window, 'outerHeight', { get: () => 1080, configurable: true }); } catch(_) {}
    try { Object.defineProperty(window, 'screenX', { get: () => 0, configurable: true }); } catch(_) {}
    try { Object.defineProperty(window, 'screenY', { get: () => 0, configurable: true }); } catch(_) {}

    // ── High-Resolution Timer Clamping (Spectre mitigation) ───────────────
    const _perfNow = performance.now.bind(performance);
    performance.now = () => Math.floor(_perfNow() * 10) / 10;  // 100µs resolution

    const _dateNow = Date.now;
    Date.now = () => Math.floor(_dateNow() / 10) * 10;

    const _dateGetTime = Date.prototype.getTime;
    Date.prototype.getTime = function() { return Math.floor(_dateGetTime.call(this) / 10) * 10; };

    // ── Font Enumeration Blocking ─────────────────────────────────────────
    try {
        Object.defineProperty(document, 'fonts', {
            get: () => ({
                status: 'loaded', size: 0,
                check: () => true,
                load: () => Promise.resolve([]),
                ready: Promise.resolve(),
                forEach: () => {},
                keys: function*() {}, values: function*() {}, entries: function*() {},
                [Symbol.iterator]: function*() {}
            }),
            configurable: true
        });
    } catch(_) {}

    // ── Network Information API ───────────────────────────────────────────
    try {
        Object.defineProperty(navigator, 'connection', {
            get: () => ({
                effectiveType: '4g', rtt: 50, downlink: 10,
                saveData: false, type: 'wifi',
                addEventListener: () => {}, removeEventListener: () => {}
            }), configurable: true
        });
    } catch(_) {}

    // ── Speech Synthesis Blocking ─────────────────────────────────────────
    try {
        if (window.speechSynthesis) {
            window.speechSynthesis.getVoices = () => [];
        }
        if (window.SpeechSynthesisUtterance) {
            window.SpeechSynthesisUtterance = undefined;
        }
    } catch(_) {}

    // ── WebRTC IP Leak Prevention ─────────────────────────────────────────
    if (window.RTCPeerConnection) {
        const _RTC = window.RTCPeerConnection;
        function PatchedRTC(config, opts) {
            if (!config) config = {};
            config.iceServers = [];
            config.iceTransportPolicy = 'relay';
            config.rtcpMuxPolicy = 'require';
            const pc = new _RTC(config, opts);
            const _createOffer = pc.createOffer.bind(pc);
            pc.createOffer = function(opts2) {
                if (opts2) { opts2.offerToReceiveAudio = false; opts2.offerToReceiveVideo = false; }
                return _createOffer(opts2);
            };
            return pc;
        }
        PatchedRTC.prototype = _RTC.prototype;
        Object.defineProperties(PatchedRTC, Object.getOwnPropertyDescriptors(_RTC));
        try { window.RTCPeerConnection = PatchedRTC; } catch(_) {}
        try { window.webkitRTCPeerConnection = PatchedRTC; } catch(_) {}
    }
    // Completely nuke MediaDevices enumeration
    if (navigator.mediaDevices) {
        navigator.mediaDevices.enumerateDevices = () => Promise.resolve([]);
    }

    // ── Keyboard Timing Coarsening ────────────────────────────────────────
    const _addEL = EventTarget.prototype.addEventListener;
    EventTarget.prototype.addEventListener = function(type, fn, opts) {
        if ((type === 'keydown' || type === 'keyup' || type === 'keypress') && typeof fn === 'function') {
            const wrapped = function(e) {
                try { Object.defineProperty(e, 'timeStamp', { get: () => Math.round(e.timeStamp), configurable: true }); } catch(_) {}
                return fn.call(this, e);
            };
            return _addEL.call(this, type, wrapped, opts);
        }
        return _addEL.call(this, type, fn, opts);
    };

    // ── Sensor APIs Blocking ──────────────────────────────────────────────
    ['DeviceMotionEvent', 'DeviceOrientationEvent', 'DeviceProximityEvent',
     'DeviceLightEvent', 'AmbientLightSensor', 'Gyroscope', 'Accelerometer',
     'LinearAccelerationSensor', 'RelativeOrientationSensor', 'AbsoluteOrientationSensor',
     'Magnetometer', 'GravitySensor'
    ].forEach(api => {
        try { if (window[api]) window[api] = undefined; } catch(_) {}
    });

    // ── Misc Privacy Hardening ────────────────────────────────────────────
    // Remove webdriver flag
    try { Object.defineProperty(navigator, 'webdriver', { get: () => false, configurable: true }); } catch(_) {}
    // Remove automation flag
    try { delete window.cdc_adoQpoasnfa76pfcZLmcfl_Array; } catch(_) {}
    try { delete window.cdc_adoQpoasnfa76pfcZLmcfl_Promise; } catch(_) {}
    try { delete window.cdc_adoQpoasnfa76pfcZLmcfl_Symbol; } catch(_) {}

    // Disable SharedArrayBuffer (Spectre)
    try { window.SharedArrayBuffer = undefined; } catch(_) {}

    // Disable cross-origin timing
    if (window.PerformanceObserver) {
        const _PO = window.PerformanceObserver;
        window.PerformanceObserver = class extends _PO {
            observe(opts) {
                if (opts && opts.entryTypes) {
                    opts.entryTypes = opts.entryTypes.filter(
                        t => t !== 'resource' && t !== 'navigation' && t !== 'longtask'
                    );
                }
                if (opts && opts.entryTypes && opts.entryTypes.length > 0)
                    super.observe(opts);
            }
        };
    }

    console.log('[Phantom] %cPrivacy Shields Active', 'color:#00ff88;font-weight:bold');
})();
)JS";
