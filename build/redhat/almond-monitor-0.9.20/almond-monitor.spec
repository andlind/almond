%global _hardened_build 1
%global _annotated_build 1
%global _package_note_flags -Wl,--build-id
%global _lto_cflags -flto -fno-fat-lto-objects
%global optflags -O2 -g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector-strong -fPIE

%define name almond-monitor
%define version 0.9.20
%define _build_id_links none

Name:           %{name}
Version:        %{version}
Release:        %{?with_avro:1.avro}%{!?with_avro:1}%{?dist}
Summary:        Almond monitoring

Group:          Applications/System
License:        GPL
URL:            https://github.com/andlind/howru
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  make, json-c-devel, librdkafka-devel, openssl-devel, zlib-devel
# Conditional BuildRequires
%{?with_avro:BuildRequires: libserdes}
Requires:       python3, python3-yaml, python3-simplejson, python3-flask, python3-gunicorn, python3-cryptography, ksh, sysstat, json-c, librdkafka
Requires(pre):  shadow-utils

%description
Almond scheduler and Howru API, compatible with Nagios plugins

%global debug_package %{nil}

%prep
%setup -q

%build
%configure %{?with_avro:--enable-avro} --prefix=/opt/almond
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
install -m 0750 %{buildroot}/usr/bin/almond %{buildroot}/opt/almond/
rm -f %{buildroot}/usr/bin/almond
install -m 0644 -D almond.conf %{buildroot}/etc/almond/almond.conf
install -m 0644 -D plugins.conf %{buildroot}/etc/almond/plugins.conf
install -m 0644 -D aliases.conf %{buildroot}/etc/almond/aliases.conf
install -m 0644 -D users.conf %{buildroot}/etc/almond/users.conf
install -m 0644 -D memalloc.conf %{buildroot}/etc/almond/memalloc.conf
install -m 0644 -D kafka.conf.example %{buildroot}/etc/almond/kafka.conf.example
install -m 0600 -D auth2fa.enc %{buildroot}/etc/almond/auth2fa.enc
install -m 0644 -D tokens %{buildroot}/etc/almond/tokens
install -m 0755 -D gardener.py %{buildroot}/opt/almond/gardener.py
install -m 0644 -D metrics.template %{buildroot}/opt/almond/templates/metrics.template
install -m 0755 -D howru %{buildroot}/opt/almond/howru
install -m 0600 -D apicmd.inf %{buildroot}/opt/almond/api_cmd/apicmd.inf
cp -r www/* %{buildroot}/opt/almond/www/
cp rs.sh %{buildroot}/opt/almond/www/api/
cp -r system/* %{buildroot}/lib/systemd/system/
cp -a plugins/* %{buildroot}/opt/almond/plugins/
cp -r utilities/* %{buildroot}/opt/almond/utilities/

#Enable mods
cp -p www/api/mods/modxml.py %{buildroot}/opt/almond/www/api/mods/enabled/modxml.py
cp -p www/api/mods/modyaml.py %{buildroot}/opt/almond/www/api/mods/enabled/modyaml.py

%files
%global default_attr 0640 almond almond
%exclude /opt/almond/www/api/mods/enabled/modxml.py
%exclude /opt/almond/www/api/mods/enabled/modyaml.py
%exclude /opt/almond/www/api/rs.sh
%exclude /opt/almond/gardener.py
%defattr(755,almond,almond,755)
/opt/almond/www/*
%config(noreplace) %attr(0644,almond,almond) /etc/almond/almond.conf
%config(noreplace) %attr(0644,almond,almond) /etc/almond/users.conf
%config(noreplace) %attr(0644,almond,almond) /etc/almond/plugins.conf
%config(noreplace) %attr(0644,almond,almond) /etc/almond/memalloc.conf
%config(noreplace) %attr(0644,almond,almond) /etc/almond/aliases.conf
%config(noreplace) %attr(0644,almond,almond) /etc/almond/kafka.conf.example
%config(noreplace) %attr(0644,almond,almond) /etc/almond/auth2fa.enc
%config(noreplace) %attr(0644,almond,almond) /etc/almond/tokens
%attr(0750, almond, almond) /opt/almond/almond
%attr(0750, almond, almond) /opt/almond/howru
%attr(0600, almond, almond) /opt/almond/api_cmd/apicmd.inf
%attr(0755,almond,almond) /opt/almond/gardener.py
%attr(0755,almond,almond) /opt/almond/www/api/rs.sh
%attr(0755,almond,almond) /opt/almond/utilities/almond-token-generator
%attr(0755,almond,almond) /opt/almond/utilities/almond-collector
%attr(0755,almond,almond) /opt/almond/utilities/check_almond
%attr(0750,almond,almond) /opt/almond/utilities/howru-user-admin.py
%attr(0750,almond,almond) /opt/almond/utilities/token-to-user.py
%attr(0644,almond,almond) /opt/almond/templates/metrics.template
%attr(0755,almond,almond) /var/log/almond/
%attr(0755,almond,almond) /opt/almond/plugins/check_overcr
%attr(0755,almond,almond) /opt/almond/plugins/check_flexlm
%attr(0755,almond,almond) /opt/almond/plugins/check_uptime.sh
%attr(0755,almond,almond) /opt/almond/plugins/check_mem.py
%attr(0755,almond,almond) /opt/almond/plugins/urlize
%attr(0755,almond,almond) /opt/almond/plugins/check_sensors
%attr(0755,almond,almond) /opt/almond/plugins/check_ifstatus
%attr(0755,almond,almond) /opt/almond/plugins/check_users
%attr(0755,almond,almond) /opt/almond/plugins/check_by_ssh
%attr(0755,almond,almond) /opt/almond/plugins/check_log
%attr(0755,almond,almond) /opt/almond/plugins/check_ircd
%attr(0755,almond,almond) /opt/almond/plugins/check_breeze
%attr(0755,almond,almond) /opt/almond/plugins/check_nagios
%attr(0755,almond,almond) /opt/almond/plugins/check_real
%attr(0755,almond,almond) /opt/almond/plugins/check_ifoperstatus
%attr(0755,almond,almond) /opt/almond/plugins/negate
%attr(0755,almond,almond) /opt/almond/plugins/check_open_files.pl
%attr(0755,almond,almond) /opt/almond/plugins/check_process
%attr(0755,almond,almond) /opt/almond/plugins/check_supervisorctl.sh
%attr(0755,almond,almond) /opt/almond/plugins/check_http
%attr(0755,almond,almond) /opt/almond/plugins/check_fping
%attr(0755,almond,almond) /opt/almond/plugins/check_ntp
%attr(0755,almond,almond) /opt/almond/plugins/check_ssl_validity
%attr(0755,almond,almond) /opt/almond/plugins/check_disk_smb
%attr(0755,almond,almond) /opt/almond/plugins/check_cluster
%attr(0755,almond,almond) /opt/almond/plugins/check_snmp
%attr(0755,almond,almond) /opt/almond/plugins/check_mysql
%attr(0755,almond,almond) /opt/almond/plugins/check_ldap
%attr(0755,almond,almond) /opt/almond/plugins/check_dhcp
%attr(0755,almond,almond) /opt/almond/plugins/check_smtp
%attr(0755,almond,almond) /opt/almond/plugins/check_dig
%attr(0755,almond,almond) /opt/almond/plugins/check_ntp_time
%attr(0755,almond,almond) /opt/almond/plugins/check_pgsql
%attr(0755,almond,almond) /opt/almond/plugins/check_load
%attr(0755,almond,almond) /opt/almond/plugins/utils.sh
%attr(0755,almond,almond) /opt/almond/plugins/check_dummy
%attr(0755,almond,almond) /opt/almond/plugins/check_procs
%attr(0755,almond,almond) /opt/almond/plugins/check_mailq
%attr(0755,almond,almond) /opt/almond/plugins/check_wave
%attr(0755,almond,almond) /opt/almond/plugins/check_hpjd
%attr(0755,almond,almond) /opt/almond/plugins/check_tcp
%attr(0755,almond,almond) /opt/almond/plugins/check_disk
%attr(0755,almond,almond) /opt/almond/plugins/check_nt
%attr(0755,almond,almond) /opt/almond/plugins/check_time
%attr(0755,almond,almond) /opt/almond/plugins/check_swap
%attr(0755,almond,almond) /opt/almond/plugins/check_apt
%attr(0755,almond,almond) /opt/almond/plugins/check_mysql_query
%attr(0755,almond,almond) /opt/almond/plugins/check_file_age
%attr(0755,almond,almond) /opt/almond/plugins/check_ntp_peer
%attr(0755,almond,almond) /opt/almond/plugins/check_mrtgtraf
%attr(0755,almond,almond) /opt/almond/plugins/check_game
%attr(0755,almond,almond) /opt/almond/plugins/check_memory2.py
%attr(0755,almond,almond) /opt/almond/plugins/check_mrtg
%attr(0755,almond,almond) /opt/almond/plugins/check_icmp
%attr(0755,almond,almond) /opt/almond/plugins/check_dns
%attr(0755,almond,almond) /opt/almond/plugins/check_cpu_stats.sh
%attr(0755,almond,almond) /opt/almond/plugins/check_nwstat
%attr(0755,almond,almond) /opt/almond/plugins/check_uptime
%attr(0755,almond,almond) /opt/almond/plugins/check_oracle
%attr(0755,almond,almond) /opt/almond/plugins/check_redpanda_metrics.py
%attr(0755,almond,almond) /opt/almond/plugins/check_ide_smart
%attr(0755,almond,almond) /opt/almond/plugins/utils.pm
%attr(0755,almond,almond) /opt/almond/plugins/check_memory.py
%attr(0755,almond,almond) /opt/almond/plugins/check_ups
%attr(0755,almond,almond) /opt/almond/plugins/check_ping
%attr(0755,almond,almond) /opt/almond/plugins/check_rpc
%attr(0755,almond,almond) /opt/almond/plugins/check_memory1.py
%attr(0755,almond,almond) /opt/almond/plugins/check_ssh
/opt/almond/plugins/check_nntp
/opt/almond/plugins/check_nntps
/opt/almond/plugins/check_ldaps
/opt/almond/plugins/check_udp
/opt/almond/plugins/check_ssmtp
/opt/almond/plugins/check_spop
/opt/almond/plugins/check_ftp
/opt/almond/plugins/check_imap
/opt/almond/plugins/check_jabber
/opt/almond/plugins/check_pop
/opt/almond/plugins/check_simap
/opt/almond/plugins/check_clamd
%attr(0750,almond,almond) /opt/almond/www/api/mods/enabled/modxml.py
%attr(0750,almond,almond) /opt/almond/www/api/mods/enabled/modyaml.py
%attr(0644,root,root) /lib/systemd/system/almond.service
%attr(0644,root,root) /lib/systemd/system/howru.service

%doc

%pre
/usr/bin/getent group almond >/dev/null || /usr/sbin/groupadd -r almond
if ! /usr/bin/getent passwd almond >/dev/null ; then
   /usr/sbin/useradd -r -g almond -d /opt/almond -s /sbin/nologin -c "User running the almomnd monitoring process" almond

fi

%postun
/usr/sbin/userdel almond 

%changelog
* Mon Nov 24 2025 0.9.19
<andreas.lindell@almondmonitor.com>
- Almond API allowed_hosts file option
- New dashboard for howru proxy
- New monitoring API for howru
* Tue Oct 28 2025 0.9.18
<andreas.lindell@almondmonitor.com>
- New parameter for HowRU plugin API call
- Buggfixes
* Tue Oct 07 2025 0.9.17
<andreas.lindell@almondmonitor.com>
- Buggfix API Kafka topic
- Refactor configs to booleans where applicable
* Thu Oct 02 2025 0.9.16
<andreas.lindell@almondmonitor.com>
- Enhanced options for Kafka producer
* Tue Sep 23 2025 0.9.15
<andreas.lindell@almondmonitor.com>
- Buggfix Almond API calls
- Improved build flow
* Wed Sep 17 2025 0.9.14
<andreas.lindell@almondmonitor.com>
- Rewritten data structure
- Security enhancements
* Tue Aug 19 2025 0.9.12
<andreas.lindell@almondmonitor.com>
- Making build secure
* Mon Aug 04 2025 0.9.11
<andreas.lindell@almondmonitor.com>
- Remove all build warnings
* Fri Jul 25 2025 0.9.10
<andreas.lindell@almondmonitor.com>
- Memory leaks covered
- Bugg fixes, updated build
* Fri Jun 27 2025 0.9.9.6-3
<andreas.lindell@almondmonitor.com>
- New status API for Almond
- Improved child thread handling
* Mon Jun 16 2025 0.9.9.6
<andreas.lindell@almondmonitor.com>
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
