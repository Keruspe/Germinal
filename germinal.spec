Name:           germinal
Version:        26
Release:        1%{?dist}
Summary:        Minimalist VTE-based terminal emulator

License:        GPL-3.0-or-later
URL:            https://github.com/Keruspe/Germinal
Source0:        %{url}/releases/download/v%{version}/%{name}-%{version}.tar.xz

BuildRequires:  meson >= 1.1
BuildRequires:  gcc
BuildRequires:  gettext
BuildRequires:  systemd-rpm-macros
BuildRequires:  pkgconfig(glib-2.0) >= 2.76
BuildRequires:  pkgconfig(gio-2.0) >= 2.76
BuildRequires:  pkgconfig(gtk4) >= 4.8
BuildRequires:  pkgconfig(vte-2.91-gtk4) >= 0.78.0
BuildRequires:  pkgconfig(libadwaita-1) >= 1.0
BuildRequires:  pkgconfig(pango)
BuildRequires:  pkgconfig(libpcre2-8)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(systemd)
BuildRequires:  pkgconfig(gsettings-desktop-schemas) >= 3.28.1

Requires:       gsettings-desktop-schemas
Recommends:     tmux

%description
Germinal is a minimalist terminal emulator based on VTE. It relies on
tmux for tabs and pane management, and uses GSettings for configuration.

%prep
%autosetup

%build
%meson
%meson_build

%check
%meson_test

%install
%meson_install
%find_lang Germinal

%files -f Germinal.lang
%license COPYING
%doc README.md
%{_bindir}/germinal
%{_datadir}/applications/org.gnome.Germinal.desktop
%{_datadir}/glib-2.0/schemas/org.gnome.Germinal.gschema.xml
%{_datadir}/metainfo/org.gnome.Germinal.metainfo.xml
%{_datadir}/dbus-1/services/org.gnome.Germinal.service
%{_userunitdir}/org.gnome.Germinal.service

%changelog
* Wed May 21 2026 Marc-Antoine Perennou <Marc-Antoine@Perennou.com> - 27-1
- Port to GTK4 and libadwaita
