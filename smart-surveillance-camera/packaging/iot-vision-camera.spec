Name:       iot-vision-camera
%define dashboard iot-dashboard
Summary:    IoT Vision Camera App
Group:      Applications/Core Applications
Version:    0.0.1
Release:    1
License:    Apache-2.0
Provides:   %{name} = %{version}-%{release}
Source0:    %{name}-%{version}.tar.gz
Source1:    %{dashboard}.service

BuildRequires: cmake
BuildRequires: hash-signer
BuildRequires: pkgconfig(ecore)
BuildRequires: pkgconfig(capi-appfw-service-application)
BuildRequires: pkgconfig(capi-system-peripheral-io)
BuildRequires: pkgconfig(libtzplatform-config)
BuildRequires: pkgconfig(gio-2.0)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(capi-media-camera)
BuildRequires: pkgconfig(capi-media-vision)
BuildRequires: pkgconfig(capi-media-image-util)
BuildRequires: pkgconfig(rpc-port)
BuildRequires: pkgconfig(libexif)
BuildRequires: pkgconfig(capi-appfw-package-manager)

%description
IoT Vision Camera Application

%prep
%setup -q

%build

%ifarch %{arm}
export CFLAGS="$CFLAGS -DTIZEN_BUILD_TARGET"
export CXXFLAGS="$CXXFLAGS -DTIZEN_BUILD_TARGET"
export FFLAGS="$FFLAGS -DTIZEN_BUILD_TARGET"
%else
export CFLAGS="$CFLAGS -DTIZEN_BUILD_EMULATOR"
export CXXFLAGS="$CXXFLAGS -DTIZEN_BUILD_EMULATOR"
export FFLAGS="$FFLAGS -DTIZEN_BUILD_EMULATOR"
%endif

%define _pkg_dir %{TZ_SYS_RO_APP}/%{name}
%define _pkg_shared_dir %{_pkg_dir}/shared
%define _pkg_res_dir %{_pkg_dir}/res
%define _pkg_shared_res_dir %{_pkg_shared_dir}/res
%define _sys_packages_dir %{TZ_SYS_RO_PACKAGES}
%define _dashboard_dir /opt/home/

cmake . -DP_NAME=%{name} \
	-DINSTALL_PREFIX=%{_pkg_dir} \
	-DAPP_SHARED_RES_DIR=%{_pkg_shared_res_dir} \
	-DDASH_BOARD_DIR=%{_dashboard_dir} \
	-DSYS_PACKAGES_DIR=%{_sys_packages_dir} \

make %{?jobs:-j%jobs}

%install
%make_install

mkdir -p %{buildroot}%{_unitdir}/multi-user.target.wants
install -m 0644 %SOURCE1 %{buildroot}%{_unitdir}/%{dashboard}.service
%install_service multi-user.target.wants %{dashboard}.service

%define tizen_sign 1
%define tizen_sign_base %{_pkg_dir}
%define tizen_sign_level platform
%define tizen_author_sign 1
%define tizen_dist_sign 1

%post
/bin/systemctl stop %{dashboard}
/bin/systemctl daemon-reload
/bin/systemctl start %{dashboard}

%postun
/bin/systemctl stop %{dashboard}

%files
%manifest %{name}.manifest
%license LICENSE NOTICE
%defattr(-,root,root,-)
%{_pkg_dir}/bin/%{name}
%{_pkg_dir}/lib/*
%{_pkg_dir}/author-signature.xml
%{_pkg_dir}/signature1.xml
%{_sys_packages_dir}/%{name}.xml
%{_pkg_shared_res_dir}/*.png
%{_pkg_res_dir}/*
%{_pkg_shared_res_dir}/*.json
%{_unitdir}/%{dashboard}.service
%{_unitdir}/multi-user.target.wants/%{dashboard}.service
%{_dashboard_dir}/*
