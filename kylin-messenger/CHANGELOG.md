# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2025-10-29
### Added
- Initial public release of **Kylin Messenger**.
- Complete LAN instant messaging stack: UDP discovery, TCP messaging, file
  transfer scaffolding, typing indicators, read receipts.
- Qt-based desktop UI with modern light/dark themes, system tray integration,
  emoji panel, inline file previews, AI assistant window.
- AI abstraction layer with echo test service and compliance service hooks.
- Automated build system (CMake), tests, and Debian packaging configuration.

### Notes
- Compliance checks currently use a stub service; GPU/CPU/RKNN back-ends can be
  integrated in future releases via the `IComplianceService` interface.
- Icon installation warns if `resources/icons/app_icon.png` is absent.

[1.0.0]: https://example.com/kylin-messenger/releases/tag/v1.0.0

