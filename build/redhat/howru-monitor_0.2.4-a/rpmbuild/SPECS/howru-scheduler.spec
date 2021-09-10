%define name howru-scheduler
%define version 0.2.4

Summary:	HowRU monitoring scheduler and api
Name:		%{name}	
Version: 	%{version}
Release:	1%{?dist}
Summary:	HowRU scheduler

Group:	        Applications/System 	
License:	GPL
URL:		https://github.com/andlind/howru
Source0:	%{name}-%{version}.tar.gz

BuildRequires:	gcc
BuildRequires:  make
Requires:	python, python-flask

%description
HowRU scheduler and API, compatible with Nagios plugins

%prep
%setup -q

%build
make %{?_smp_mflags}

%install
make install DESTDIR=%{?buildroot} 
mkdir -p %{buildroot}/etc/howru/
mkdir -p %{buildroot}/var/log/howru/
cp howruc.conf %{buildroot}/etc/howru/
cp plugins.conf %{buildroot}/etc/howru 
mkdir -p %{buildroot}/opt/howru/plugins
mkdir -p %{buildroot}/opt/howru/www
mkdir -p %{buildroot}/lib/systemd/system/
cp -r www/* %{buildroot}/opt/howru/www/
cp -r system/* %{buildroot}/lib/systemd/system/

%files
%attr(0644,root,root) %config(noreplace) /etc/howru/howruc.conf
%attr(0644,root,root) %config(noreplace) /etc/howru/plugins.conf
%attr(0770,root,root) /opt/howru/howru-scheduler
%attr(0755,root,root) /var/log/howru/
%attr(0755,root,root) /opt/howru/plugins/
%attr(0644,root,root) /lib/systemd/system/howru-scheduler.service
%attr(0644,root,root) /lib/systemd/system/howru-api.service
%defattr(755,root,root,755)
/opt/howru/www/*

%doc

%changelog
* Fri Sep 10 2021 Startup
<andreas.lindell@svenskaspel.se>
- HowRU rpmbuid set up
