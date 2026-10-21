/** Browser scheduler with setTimeout fallback and a monotonic clock when
    performance.now is available (Date.now otherwise). */
include Tea_core.Platform.S;
