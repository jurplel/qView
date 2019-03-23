Name:		qview
Version:	2.0
Release:	1
Summary:	Practical and minimal image viewer

Group:		Applications/Multimedia
License:	GPLv3
URL:		https://intvhq.com/qview/
Source0:	https://github.com/jurplel/qView/releases/download/%{version}/qView-%{version}.tar.gz


%description
qView is a cross-platform Qt image viewer designed to be practical and minimal

%global debug_package %{nil}

%prep
%setup -q -n qView


%build
qmake-qt5
make %{?_smp_mflags}


%install
mkdir -p %{buildroot}/usr/bin/
cp bin/qview %{buildroot}/usr/bin/
mkdir -p %{buildroot}/usr/share/icons/
cp -r dist/linux/hicolor %{buildroot}/usr/share/icons/
mkdir -p %{buildroot}/usr/share/applications
cp dist/linux/qView.desktop %{buildroot}/usr/share/applications/

%files
/usr/bin/*
/usr/share/icons/hicolor/16x16/*
/usr/share/icons/hicolor/32x32/*
/usr/share/icons/hicolor/64x64/*
/usr/share/icons/hicolor/128x128/*
/usr/share/icons/hicolor/256x256/*
/usr/share/icons/hicolor/scalable/*
/usr/share/applications/*
%license LICENSE
%doc



%changelog






