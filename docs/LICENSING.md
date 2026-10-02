<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
# Licensing and Steam distribution

## Project license

Original Arm Swing VR files are offered under GNU GPL version 3 or, at the
recipient's option, any later version (`GPL-3.0-or-later`). The complete GPLv3
text is in [LICENSE](../LICENSE); this policy explains the project's distribution
plan and does not replace or amend that license. Other authors' code and license
documents retain their original terms.

Paid distribution is compatible with GPL. The planned Steam edition sells
convenient installation and updates. It carries the same GPL permissions as other
builds, including modification and redistribution. Other people can redistribute
or sell GPL-compliant copies too. There is no noncommercial restriction,
Steam-only restriction, or proprietary edition implied by this policy.

Reference: [GNU FAQ on selling copies](https://www.gnu.org/licenses/gpl-faq.html#DoesTheGPLAllowMoney).

## Steamworks is a separate dependency decision

Valve identifies conflicts between copyleft licenses and combining software with
the proprietary Steamworks SDK. This project currently uses neither that SDK nor
the Steam DRM wrapper. Its executable is intended to run outside the Steam client
as well as when launched by Steam. Publishing a package and linking an API are
separate engineering decisions.

The public OpenVR SDK is licensed separately under BSD-3-Clause. The SteamVR
runtime is an external interoperability target, not code being relicensed by
this project. OpenXR is likewise an API/runtime boundary with separately
licensed development components.

No Steamworks linking exception is granted. If a future feature needs Steamworks
linking, resolve the rights for every relevant contribution and dependency first.
A maintainer cannot grant an exception to third-party code on its author's
behalf. Contributions under ordinary GPL terms do not automatically grant such
an exception. A separate process is not automatically a licensing exemption.

Reference: [Valve's open-source distribution guidance](https://partner.steamgames.com/doc/sdk/uploading/distributing_opensource).

## Binary release requirements

For each release, including a paid Steam build:

1. Tag the exact source revision and record the compiler, build options, pinned
   dependencies, patches, and packaging/install scripts used for the binaries.
2. Prepare complete corresponding source, including required dependency source
   and notices. A repository link alone, a moving branch, or a bare `git archive`
   is not sufficient if it omits required material. Evaluate the GPL's System
   Library exception for the actual package; do not assume all dependencies qualify.
3. Offer the matching source at no additional charge using a distribution method
   that satisfies GPLv3 section 6. The intended Steam approach is to include a
   complete source archive and build instructions in the depot, and mirror that
   exact archive with the public release. Retain historical source for every
   distributed version. Steam packaging has not yet been implemented or reviewed.
4. Include the GPL text, copyright notices, modification notices, and applicable
   third-party license/NOTICE files. Keep license and source information accessible
   from the application and distribution page.
5. For bundled Qt, preserve its LGPL permissions, provide the required Qt source,
   and allow replacement/relinking of the Qt libraries. Inventory the actual
   Qt plugins and their third-party dependencies. Dynamic linking simplifies this
   work but does not by itself satisfy every requirement.
6. Do not add product-specific EULA terms, DRM, or technical restrictions that
   take away GPL/LGPL permissions. Review the actual Steam agreement, delivery
   setup, and package before commercial release; this bootstrap is not Steam
   approval or a legal opinion on a future package.

Reference: [GPLv3, particularly sections 3, 6, 7, and 10](https://www.gnu.org/licenses/gpl-3.0.html).

## Provenance

Keep [THIRD_PARTY.md](../THIRD_PARTY.md) current. Prefer original implementation
and compatible SDKs; preserve upstream terms when reusing source. Do not import
AGPL, GPLv2-only, noncommercial, or proprietary code without resolving its effect
on the project license and distribution model. Existing repository research is
not permission to copy its implementation.
