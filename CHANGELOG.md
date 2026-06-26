# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.3.0] — 2026-06-26

### Added
- The installed version is now shown in the settings window (bottom-left) and in
  the tray icon tooltip, so you can tell which build is installed. The GUI reads
  it from the daemon (`cursorpop --version`), keeping a single source of truth.
- Custom tray icon with white and black variants, installed into the hicolor
  icon theme. A new "Tray icon" setting (Automatic / White / Black) picks the
  variant. The white variant is also used as the application icon in menus and
  window decorations. Changing the setting updates the running tray live (the
  tray watches the config file), so no restart is needed.

### Changed
- Display name is now branded **CursorPop** in titles and menu/tray entries
  (window title, `.desktop` `Name`, README/doc headings). The lowercase
  `cursorpop` is kept for the binary, package, URLs, paths and the gettext domain.

## [0.2.0] — 2026-06-25

### Added
- Internationalization (i18n) of the settings GUI via `gettext`. The window and
  tray menu follow the system language; English and Spanish are bundled, and any
  other language falls back to English.
- Translation tooling: `po/cursorpop.pot` template, `po/es.po` catalog, and
  `make mo` / `make pot` targets. `make mo` degrades gracefully when `gettext`
  is not installed.
- GitHub Actions CI: builds the daemon, compiles translations, checks the GUI
  syntax, validates the desktop files and builds the `.deb`.
- `CHANGELOG.md`, issue/PR templates under `.github/`, and an `English | Español`
  README split (`README.md` / `README.es.md`).
- `postrm` script so the desktop database is refreshed after package removal.

### Changed
- The entire project is now written in English (source comments, daemon
  `--help`/error messages, docs, man page and packaging descriptions).
- The `.desktop` files now default to English with Spanish (`[es]`) variants.
- `make uninstall` now also removes the installed translation catalogs.
- Build dependencies now include `gettext`.

### Fixed
- Corrected the package `Homepage` URL to `github.com/ecerutti/cursorpop`.

## [0.1.0] — 2026-06-20

### Added
- Initial release (Spanish). Click-shrink and shake-to-grow cursor effects for
  X11, a C daemon, a GTK settings GUI with a system tray icon, autostart from
  the GUI, a man page and Debian packaging.

[Unreleased]: https://github.com/ecerutti/cursorpop/compare/v0.3.0...HEAD
[0.3.0]: https://github.com/ecerutti/cursorpop/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/ecerutti/cursorpop/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/ecerutti/cursorpop/releases/tag/v0.1.0
