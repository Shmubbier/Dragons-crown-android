// SPDX-License-Identifier: GPL-2.0-or-later
// dcp: Android performance hints (ADPF). Tells the system which threads a frame depends on and how long
// frames take against a 60 fps target, so it gives those threads faster cores or higher clocks when
// frames run late (within its thermal limits). DCP_ADPF=0 turns it off. No-op off Android.

#pragma once

namespace DcpAdpf {

/// Once per guest flip, from the VideoOut thread.
void OnFlip();

} // namespace DcpAdpf
