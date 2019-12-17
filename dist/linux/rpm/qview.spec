Name:		qview
Version:	3.0
Release:	1
Summary:	Practical and minimal image viewer

Group:		Productivity/Graphics/Viewers
License:	GPL-3.0-only
URL:		https://interversehq.com/qview/
Source:	    https://github.com/jurplel/qView/releases/download/%{version}/qView-%{version}.tar.gz

BuildRequires: pkgconfig
BuildRequires: pkgconfig(Qt5Concurrent) >= 5.9
BuildRequires: pkgconfig(Qt5Widgets) >= 5.9
BuildRequires: pkgconfig(Qt5Network) >= 5.9


%description
qView is a Qt image viewer designed with minimalism and usability in mind.

%prep
%autosetup -n qView

%build
qmake-qt5
%make_build

%install
mkdir -p %{buildroot}/usr/bin/
cp bin/qview %{buildroot}/usr/bin/
mkdir -p %{buildroot}/usr/share/icons/GPLv3
cp -r dist/linux/hicolor %{buildroot}/usr/share/icons/
mkdir -p %{buildroot}/usr/share/applications
cp dist/linux/qView.desktop %{buildroot}/usr/share/applications/

%files
/usr/bin/*
/usr/share/icons/*
/usr/share/applications/*
%license LICENSE
%doc README.md

%changelog