%global _lto_cflags %{nil}
%define name almond-monitor
%define version 0.9.10
%define _build_id_links none

Name:           %{name}
Version:        %{version}
Release:        1.avro%{?dist}
Summary:        Almond monitoring

Group:          Applications/System
License:        GPL
URL:            https://github.com/andlind/howru
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  make, json-c-devel, librdkafka-devel, openssl-devel, zlib-devel
BuildRequires:  confluent-libserdes-devel
Requires:       python3, python3-yaml, python3-simplejson, python3-flask, python3-gunicorn, python3-cryptography, ksh, sysstat, json-c, librdkafka
Requires(pre):  shadow-utils

%description
Almond scheduler and Howru API, compatible with Nagios plugins

%global debug_package %{nil}

%prep
%setup -q

%build
%configure --prefix /opt/almond --enable-avro
make %{?_smp_mflags}

%install
make install DESTDIR=%{?buildroot}
mkdir -p %{buildroot}/etc/almond/
mkdir -p %{buildroot}/var/log/almond/
mkdir -p %{buildroot}/opt/almond/
mkdir -p %{buildroot}/opt/almond/plugins/
mkdir -p %{buildroot}/opt/almond/templates/
mkdir -p %{buildroot}/opt/almond/utilities/
mkdir -p %{buildroot}/opt/almond/www
mkdir -p %{buildroot}/opt/almond/api_cmd/
mkdir -p %{buildroot}/lib/systemd/system/
cp almond.conf %{buildroot}/etc/almond/
cp plugins.conf %{buildroot}/etc/almond/
cp aliases.conf %{buildroot}/etc/almond/
cp users.conf %{buildroot}/etc/almond/
cp memalloc.conf %{buildroot}/etc/almond/
cp tokens %{buildroot}/etc/almond/
cp auth2fa.enc %{buildroot}/etc/almond/
cp gardener.py %{buildroot}/opt/almond/
cp plugins_status.avsc %{buildroot}/opt/almond/
cp howru %{buildroot}/opt/almond/
cp memalloc.alm %{buildroot}/opt/almond/
cp apicmd.inf %{buildroot}/opt/almond/api_cmd/
cp metrics.template %{buildroot}/opt/almond/templates/
cp -r www/* %{buildroot}/opt/almond/www/
cp rs.sh %{buildroot}/opt/almond/www/api/
cp -r system/* %{buildroot}/lib/systemd/system/
cp -r plugins/* %{buildroot}/opt/almond/plugins/
cp -r utilities/* %{buildroot}/opt/almond/utilities/

#Enable mods
cp -p www/api/mods/modxml.py %{buildroot}/opt/almond/www/api/mods/enabled/modxml.py
cp -p www/api/mods/modyaml.py %{buildroot}/opt/almond/www/api/mods/enabled/modyaml.py

%files
%global default_attr 0640 almond almond
%attr(0644,almond,almond) %config(noreplace) /etc/almond/almond.conf
%attr(0644,almond,almond) %config(noreplace) /etc/almond/plugins.conf
%attr(0644,almond,almond) %config(noreplace) /etc/almond/aliases.conf
%attr(0644,almond,almond) %config(noreplace) /etc/almond/users.conf
%attr(0644,almond,almond) %config(noreplace) /etc/almond/memalloc.conf
%attr(0644,almond,almond) %config(noreplace) /etc/almond/tokens
%attr(0600,almond,almond) %config(noreplace) /etc/almond/auth2fa.enc
%attr(0600,almond,almond) /opt/almond/memalloc.alm
%attr(0640,almond,almond) /opt/almond/plugins_status.avsc
%attr(0600,almond,almond) /opt/almond/api_cmd/apicmd.inf
%attr(0755,almond,almond) /opt/almond/gardener.py
%attr(0755,almond,almond) /opt/almond/howru
%attr(0755,almond,almond) /opt/almond/utilities/almond-token-generator
%attr(0755,almond,almond) /opt/almond/utilities/almond-collector
%attr(0755,almond,almond) /opt/almond/utilities/check_almond
%attr(0750,almond,almond) /opt/almond/utilities/howru-user-admin.py
%attr(0750,almond,almond) /opt/almond/utilities/token-to-user.py
%attr(0644,almond,almond) /opt/almond/templates/metrics.template
%attr(0770,almond,almond) /opt/almond/almond
%attr(0755,almond,almond) /var/log/almond/
%attr(0755,almond,almond) /opt/almond/plugins/
%attr(0644,root,root) /lib/systemd/system/almond.service
%attr(0644,root,root) /lib/systemd/system/howru.service
%defattr(755,almond,almond,755)
/opt/almond/www/*
%exclude /opt/almond/www/api/rs.sh
%exclude /opt/almond/www/api/mods
%exclude /opt/almond/www/api/mods/*
%attr(0755,almond,almond) /opt/almond/www/api/rs.sh
%attr(0750,almond,almond) /opt/almond/www/api/mods/

%doc

%pre
/usr/bin/getent group almond >/dev/null || /usr/sbin/groupadd -r almond
if ! /usr/bin/getent passwd almond >/dev/null ; then
   /usr/sbin/useradd -r -g almond -d /opt/almond -s /sbin/nologin -c "User running the almomnd monitoring process" almond

fi

%postun
/usr/sbin/userdel almond 

%changelog
* Fri Jul 18 2025 0.9.10
<andreas.lindell@almondmonitor.com>
- Updated build structure
- Compile, build with or without avro support
* Thu Jul 03 2025 0.9.9.8
- Improved API socket handling
* Wed Jul 02 2025 0.9.9.7-8
- Update of documentation
- Update versioning
* Tue Jul 01 2025 0.9.9.7
- Avro support for Kafka producer
* Fri Jun 27 2025 0.9.9.6-3
- New status API for Almond
- Improved child thread handling
* Mon Jun 16 2025 0.9.9.6
- HowRu two-factor auth
- New tools in utilities
- Buggfixes
* Fri May 30 2025 0.9.9.5
<andreas.lindell@almondmonitor.com>
- Almond option to use external scheduler
- Source code change logging as own code element
* Wed May 28 2025 0.9.9.4
<andreas.lindell@almondmonitor.com>
- Bugfix and improved code in Almond
* Fri May 23 2025 0.9.9.3
<andreas.lindell@almondmonitor.com>
- Adding log tab to Howru API admin page
- Adding howru token to installation
- Small buggfixes
* Mon Jan 20 2025 0.9.9
<andreas.lindell@almondmonitor.com
- Code refactoring, bugg fixes
- HowRU updates, integration API:s
* Mon Jan 13 2025 0.9.8
<andreas.lindell@almondmonitor.com>
- New Almond API commands
* Thu Dec 19 2024 0.9.7.2
<andreas.lindell@almondmonitor.com>
- Update config read in HowRu
- HowRu now run in Gunicorn
* Wed Dec 11 2024 0.9.7
<andreas.lindell@almondmonitor.com>
- HowRU admin page update
- HowRU prom file export in multi mode
*Thu Nov 28 2024 0.9.6.4
<andreas.lindell@almondmonitor.com>
- Improvements clock based scheduler
- Improved rpm build
*Thu Nov 21 2024 0.9.6
<andreas.lindell@almondmonitor.com>
- New clock based scheduler
- 0.9.5 Code refactor
- Buggfixes
*Thu Sep 26 2024 0.9.4-3
- Bugg fixes
<andreas.lindell@almondmonitor.com>
*Tue Sep 17 2024 0.9.4
<andreas.lindell@almondmonitor.com>
- New mods function for HowRU API
*Fri Sep 06 2024 0.9.3
<andreas.lindell@almondmonitor.com>
- HowRU admin page update API calls
* Tue Aug 20 2024 0.9.2
<andreas.lindell@almondmonitor.com>
- New API:s for HowRU integration
- TLS option for Almond API
* Thu Jul 11 2024 0.9.1
<andreas.lindell@almondmonitor.com>
- Rewritten memalloc function
- Buggfixes
* Thu Jun 27 2024 0.9.0
<andreas.lindell@almondmonitor.com>
- apicmd function
- extended Almond API
* Thu May 23 2024 0.8.3
<andreas.lindell@almondmonitor.com>
- Buggfixes
- Some updates in documentation
* Mon Mar 25 2024 0.8.0
<andreas.lindell@almondmonitor.com>
- Added items to Almond API
- Dynamic memory allocation
- Bugg fixes
- Hard reload of conf, temp feature
* Tue Mar 05 2024 0.7.1
<andreas.lindell@almondmonitor.com>
- Bugg fixes
- API query search in multimode
- Extended Kafka features
* Fri Mar 01 2024 Version 0.7.0
<andreas.lindell@almondmonitor.com>
- New features in Almond API
- New utilities
* Wed Feb 28 2024 Version 0.6.3
<andreas.lindell@almondmonitor.com>
- Almond data cache clear
- Buggfixes
* Tue Feb 20 2024 Version 0.6.2
<andreas.lindell@hotmail.com>
- Kafka producer SSL
- Kafka tags 
- Buggfixes
* Thu Aug 10 2023 Version 0.6
<andreas.lindell@hotmail.com>
- Kafka producer functionality
- Buggfixes
* Thu May 04 2023 Version 0.5
<andreas.lindell@hotmail.com>
- Almond socket api
- Quick start
- Run plugins from admin gui
- Buggfixes
* Wed Apr 12 2023 Update
<andreas.lindell@hotmail.com>
- Metrics multimode
- Buggfixes and improvements
* Tue Mar 14 2023 Almond
<andreas.lindell@hotmail.com>
- Admin gui
- Bugg fixes
- Rename scheduler to almond
- Run almond as user
* Fri Feb 03 2023 Update
<andreas.lindell@hotmail.com>
- Prometheus export
- Update small gui
- Bugg fixes
* Tue Nov 29 2022 Metrics collection
<andreas.lindell@hotmail.com>
- Option to collect metrics instead of json
* Wed Jun 29 2022 Docker logs option
<andreas.lindell@hotmail.com>
- Option to log to stdout
* Wed Jun 22 2022 Api server flag added
<andreas.lindell@hotmail.com>
- Option to use servername argument with api
* Fri Jun 03 2022 Store feature
<andreas.lindell@hotmail.com>
- Option to store plugin results
* Tue May 31 2022 Serverfile
<andreas.lindell@hotmail.com>
- Adding server file feature
* Wed Feb 09 2022 Multimode
<andreas.lindell@hotmail.com>
- Multimode added
* Fri Sep 10 2021 Startup
<andreas.lindell@hotmail.com>
- HowRU rpmbuid set up
