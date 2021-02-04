// ZQuery.js
// minimal slice of JQuery functionality

(function (global) {
    const _noop = function () { }
    const local_alert = global.alert ? global.alert : _noop;

    if (!global.document) {
        local_alert("no document property: further JS won't work");
        return;
    }
    if (!global.document.createElement) {
        local_alert("no document.querySelectorAll property: further JS won't work");
        return;
    }
    if (!global.document.querySelectorAll) {
        local_alert("no document.querySelectorAll property: further JS won't work");
        return;
    }
    if (!global.Array.isArray) {
        local_alert("no Array.isArray function: further JS won't work");
        return;
    }

    function is_string(x) {
        "use strict";
        var type_x = typeof (x);
        if ("string" === type_x) return true;
        if ("object" === type_x) {
            return x instanceof global.String;
        }
        return false;
    }

    // want to know if countup loop is valid
    function is_countloop_valid(x) {
        "use strict";
        if (global.Array.isArray(x)) return true;
        if (x instanceof global.NodeList) return true;
        return false;
    }

    function DOMappend(dest_zq, src) {
        "use strict";
        if (src instanceof global.Node) {
            dest_zq.push(src);
            return true;
        }
        if (is_countloop_valid(src)) {
            const ub = src.length;
            let i = 0;
            while (i < ub) DOMappend(dest_zq, src[i++]);
            return true;
        }
        return false;
    }

    // returns something array-ish
    global.ZQuery = function (selector, context) {
        if (!selector) return;
        if (is_string(selector)) {
            selector = selector.trim();

            if ('<' === selector[0]) {
                const staging = global.document.createElement('span');
                staging.innerHTML = selector;
                DOMappend(this, staging.childNodes);
                return;
            } else {
                // selector-ish
                if ("undefined" === typeof (context)) context = global.document;
                if (context.querySelectorAll) {
                    try {
                        DOMappend(this, context.querySelectorAll(selector));
                        return;
                    } catch (e) {
                        local_alert("illegal selector: " + selector);
                        throw e; // unlike C++, this does not lose call chain information
                    }
                }
            }
        } else if (selector instanceof global.ZQuery) {
            // value copy construction
            const ub = selector.length;
            let n = 0;
            this.length = ub;
            while (n < ub) {
                this[n] = selector[n];
                n++;
            };
            return;
        } else if (DOMappend(this, selector)) return;
        throw new Error("unhandled selector");
    };

    global.ZQuery.prototype = Object.create(global.Array.prototype);
    global.ZQuery.prototype.constructor = global.ZQuery;
    global.ZQuery.prototype.noop = _noop;

    global.ZQuery.prototype.base_map = global.ZQuery.prototype.map; // just in case we need Array.map

    // ideally Element fn(Element, index)
    global.ZQuery.prototype.map = function(callback) {
        "use strict";
        const ub = this.length;
        const staging = [];
        let n = 0;
        while (n < ub) {
            const src = this[n++];
            if (null === src) continue;
            if ("undefined" === typeof(src)) continue;
            DOMappend(staging, callback(src, n - 1));
        }
        return new ZQuery(staging);
    }

    function DOMparent(src) {
        "use strict";
        if (src.parentElement) return src.parentElement;
        if (src.parentNode) return src.parentNode;
        return null;
    }

    global.ZQuery.prototype.parent = function () {
        "use strict";
        return this.map(DOMparent);
    }

    function DOMancestor_match(src, selector) {
        "use strict";
        while (src) {
            if (src.matches && src.matches(selector)) return src; // can throw, but want to fix the selector rather than cruise on
            if (src.parentElement) src = src.parentElement;
            else if (src.parentNode) src = src.parentNode;
            else break;
        }
        return null;
    }

    global.ZQuery.prototype.closest = function (selector) {
        "use strict";
        return this.map(function (src) { return DOMancestor_match(src, selector); });
    }

    global.ZQuery.prototype.css = function (attr, value) {
        const ub = this.length;
        if (0 >= ub) return;
        if ("undefined" === typeof (value) || null === value) {
            const st = global.window.getComputedStyle(test);
            if (attr in st) return st[attr];
            // \todo interpose CSS -> JavaScript translation here
            return;
        }
        let n = 0;
        while (ub > n) {
            const test = this[n++];
            if (test.style) test.style[attr] = value;
        }
    }
})(this);
