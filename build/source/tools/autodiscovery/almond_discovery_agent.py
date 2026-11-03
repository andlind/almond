#!/usr/bin/env python3
"""Generate an initial Almond monitoring configuration for Linux.

By default print the configuration to stdout and diagnostics to stderr.
--apply writes the target; --force is required to replace an existing target
and first creates a timestamped backup. Generated checks are executed once
before output or installation. Run with the same user as Almond if possible.
See --help for options. Python 3.8+; standard library only.
"""
import argparse
import datetime
import hashlib
import ipaddress
import json
import os
from pathlib import Path
import re
import signal
import shlex
import shutil
import socket
import struct
import subprocess
import sys
import tempfile

HEADER = '# [service_name] description;plugin args;active;interval labels'
PSEUDO = {'proc', 'sysfs', 'devtmpfs', 'devpts', 'tmpfs', 'cgroup', 'cgroup2',
          'securityfs', 'pstore', 'debugfs', 'tracefs', 'configfs', 'fusectl',
          'mqueue', 'hugetlbfs', 'autofs', 'nsfs', 'bpf', 'binfmt_misc',
          'rpc_pipefs', 'efivarfs', 'ramfs', 'squashfs'}
# Exact Linux process names and recognized specialty-plugin names, never arbitrary commands.
SERVICES = {
    'postgresql': (('postgres', 'postmaster'), ('check_pgsql', 'check_postgres', 'check_postgres.pl', 'check_postgresql')),
    'mysql': (('mysqld', 'mariadbd'), ('check_mysql', 'check_mysql_health', 'check_mysql_health.pl')),
    'redis': (('redis-server',), ('check_redis', 'check_redis.py', 'check_redis.pl')),
    'nginx': (('nginx',), ('check_nginx', 'check_nginx.py', 'check_nginx.sh')),
    'apache': (('apache2', 'httpd'), ('check_apache', 'check_apache.py', 'check_http')),
    'haproxy': (('haproxy',), ('check_haproxy', 'check_haproxy.py')),
    'postfix': (('master',), ('check_postfix', 'check_postfix.pl')),
    'dovecot': (('dovecot',), ('check_dovecot', 'check_imap')),
    'exim': (('exim', 'exim4'), ('check_exim',)),
    'tomcat': (('java',), ('check_tomcat', 'check_tomcat.py')),
    'jetty': (('java',), ('check_jetty',)),
    'gunicorn': (('gunicorn', 'python', 'python3', 'python3.11', 'python3.12', 'python3.13'), ('check_gunicorn', 'check_http')),
    'supervisord': (('supervisord', 'python', 'python3', 'python3.11', 'python3.12', 'python3.13'), ('check_supervisord', 'check_supervisor')),
    'uwsgi': (('uwsgi',), ('check_uwsgi',)),
    'docker': (('dockerd',), ('check_docker', 'check_docker.py')),
    'containerd': (('containerd',), ('check_containerd',)),
    'mongodb': (('mongod',), ('check_mongodb', 'check_mongodb.py')),
    'memcached': (('memcached',), ('check_memcached',)),
    'prometheus': (('prometheus',), ('check_prometheus',)),
    'node_exporter': (('node_exporter',), ('check_node_exporter',)),
    'elasticsearch': (('java',), ('check_elasticsearch', 'check_elasticsearch.py')),
    'rabbitmq': (('beam.smp',), ('check_rabbitmq', 'check_rabbitmq.py')),
    'howru': (('python', 'python3', 'gunicorn'), ('check_howru_api.py',)),
    'redpanda': (('redpanda',), ('check_redpanda', 'check_kafka')),
    'minio': (('minio',), ('check_minio', 'check_minio.py')),
    'flume': (('java',), ('check_flume', 'check_flume.py')),
    'sybase': (('dataserver',), ('check_sybase', 'check_sybase.pl', 'check_sybase_health')),
    'nghttpx': (('nghttpx',), ('check_nghttpx',)),
    'chrony': (('chronyd',), ('check_chrony', 'check_ntp_time')),
    'dnsmasq': (('dnsmasq',), ('check_dns',)),
    'snmpd': (('snmpd',), ('check_snmp',)),
    'nrpe': (('nrpe',), ('check_nrpe',)),
    'collectd': (('collectd',), ('check_collectd',)),
    'rsyslog': (('rsyslogd',), ('check_rsyslog',)),
    'kafka': (('java',), ('check_kafka', 'check_kafka.py')),
    'sshd': (('sshd',), ('check_ssh',)),
    'almond': (('almond',), ('check_almond_ps',)),
}
# Strings must be present in the actual command line for ambiguous process names.
CMD_REQUIRED = {'postfix': ('postfix',), 'tomcat': ('catalina', 'tomcat'),
                'jetty': ('jetty',), 'kafka': ('kafka.kafka', 'kafka.kafkaservers', 'kafka.kraft'),
                'elasticsearch': ('elasticsearch',), 'rabbitmq': ('rabbitmq',),
                'howru': ('howru.py', 'howru:app', 'howru.app'),
                'flume': ('org.apache.flume.node.application',)}
SAFE_NAME = re.compile(r'^[A-Za-z0-9_.+-]+$')
MOUNT_SAFE = re.compile(r'^/[A-Za-z0-9_./+@=-]*$')


def read_text(path):
    try:
        return Path(path).read_text(errors='replace').strip()
    except OSError:
        return ''


def command_output(args):
    try:
        p = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                           text=True, timeout=2, check=False)
        return p.stdout.strip() if p.returncode == 0 else ''
    except (OSError, subprocess.TimeoutExpired):
        return ''


def environment():
    cgroup = read_text('/proc/1/cgroup').lower()
    container = ('docker' if Path('/.dockerenv').exists() else
                 'podman' if Path('/run/.containerenv').exists() else '')
    if not container:
        for word in ('kubepods', 'containerd', 'docker', 'lxc', 'podman'):
            if word in cgroup:
                container = word
                break
    if not container:
        container = command_output(['systemd-detect-virt', '--container'])
    vendor = (read_text('/sys/class/dmi/id/sys_vendor') + ' ' +
              read_text('/sys/class/dmi/id/product_name')).lower()
    vm = command_output(['systemd-detect-virt', '--vm'])
    if 'vmware' in vendor or vm == 'vmware':
        vm = 'vmware'
    elif not vm:
        for token in ('kvm', 'qemu', 'virtualbox', 'xen', 'hyper-v'):
            if token in vendor:
                vm = token
                break
    # A container can run on a VM; do not assert physical hardware when unknown.
    if container:
        return 'container: ' + container + ('; virtualisation visible: ' + vm if vm else '')
    if vm:
        return 'virtual machine: ' + vm
    if vendor.strip() and not any(x in vendor for x in ('virtual', 'unknown', 'not specified')):
        return 'probably physical server (DMI: ' + vendor.strip() + ')'
    return 'unknown (physical server or undetected VM)'


def mountpoints(path='/proc/self/mountinfo'):
    found = set()
    for line in read_text(path).splitlines():
        before, sep, after = line.partition(' - ')
        left, right = before.split(), after.split()
        if not sep or len(left) < 5 or len(right) < 2:
            continue
        mount = re.sub(r'\\([0-7]{3})', lambda m: chr(int(m.group(1), 8)), left[4])
        fs = right[0]
        if not MOUNT_SAFE.fullmatch(mount) or not os.path.isdir(mount):
            continue
        if mount != '/' and (fs in PSEUDO or mount.startswith(('/proc/', '/sys/', '/dev/', '/run/'))
                             or mount in ('/proc', '/sys', '/dev', '/run')):
            continue
        found.add(mount)
    return sorted(found, key=lambda p: (p != '/', p))


def processes(proc='/proc'):
    result = []
    try:
        entries = list(Path(proc).iterdir())
    except OSError:
        return result
    for entry in entries:
        if not entry.name.isdigit():
            continue
        name = read_text(entry / 'comm')
        try:
            cmd = (entry / 'cmdline').read_bytes().replace(b'\x00', b' ').decode('utf-8', 'replace').strip()
        except OSError:
            cmd = ''
        if name and cmd:  # avoid counting zombies and kernel threads
            result.append((name, cmd))
    return result


def detected_services(procs):
    found = {}
    for service, (names, _) in SERVICES.items():
        matches = []
        for name, cmd in procs:
            if service in ('supervisord', 'gunicorn') and name.startswith('python'):
                if not re.search(r'(^|[/\s])' + service + r'(?:\s|$|\.py\b)', cmd, re.I):
                    continue
            name_matches = name in names or (service in ('supervisord', 'gunicorn') and
                                              re.fullmatch(r'python(?:[0-9.]+)?', name))
            if name_matches and (service not in CMD_REQUIRED or
                                  any(s in cmd.lower() for s in CMD_REQUIRED[service])):
                matches.append((name, cmd))
        if matches:
            found[service] = matches
    return found


def python_apps(procs):
    """Identify Python script paths; a script name alone does not prove it is a web app."""
    found = set()
    for name, cmd in procs:
        if not re.fullmatch(r'python(?:[0-9.]+)?', name):
            continue
        for token in cmd.split()[1:]:
            if (token.startswith('/') and token.endswith('.py') and MOUNT_SAFE.fullmatch(token)
                    and not token.startswith(('/usr/bin/', '/usr/lib/', '/usr/lib64/'))
                    and token != os.path.abspath(__file__)):
                found.add((name, token))
                break
    return sorted(found)


def gunicorn_bind_ports(matches):
    """Extract explicitly configured IPv4 binds; only test ports also observed listening."""
    found = set()
    for _name, cmd in matches:
        try:
            args = shlex.split(cmd)
        except ValueError:
            continue
        for i, arg in enumerate(args):
            value = (args[i + 1] if arg in ('-b', '--bind') and i + 1 < len(args)
                     else arg.split('=', 1)[1] if arg.startswith('--bind=') else '')
            match = re.fullmatch(r'(?:0\.0\.0\.0|127\.0\.0\.1|localhost):(\d{1,5})', value)
            if match and 0 < int(match.group(1)) <= 65535:
                found.add(int(match.group(1)))
    return found


def plugins(directory):
    path = Path(directory)
    if not path.is_dir():
        raise ValueError('Plugin directory does not exist: ' + directory)
    found = {}
    for item in path.iterdir():
        if SAFE_NAME.fullmatch(item.name) and item.is_file() and os.access(item, os.X_OK):
            found[item.name] = item
    return found


def local_listening_ports(path='/proc/net/tcp'):
    ports = set()
    for line in read_text(path).splitlines()[1:]:
        fields = line.split()
        if len(fields) < 4 or fields[3] != '0A':
            continue
        try:
            addr, port = fields[1].split(':')
            # /proc/net/tcp stores IPv4 addresses as little-endian bytes.
            ip = ipaddress.IPv4Address(bytes.fromhex(addr)[::-1])
            if ip.is_unspecified or ip.is_loopback:
                ports.add(int(port, 16))
        except ValueError:
            continue
    return ports


def systemd_inventory():
    """Return running/failed service details; available inside containers if systemd runs."""
    args = ['systemctl', 'list-units', '--type=service', '--state=running,failed',
            '--no-legend', '--plain', '--no-pager']
    try:
        result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                text=True, timeout=10, check=False,
                                env=dict(os.environ, LC_ALL='C', SYSTEMD_COLORS='0'))
    except (OSError, subprocess.TimeoutExpired):
        return [], 'systemd discovery unavailable'
    if result.returncode:
        return [], 'systemd discovery unavailable (systemctl could not contact systemd)'
    units = []
    for line in result.stdout.splitlines():
        parts = line.lstrip('● ').split(None, 4)
        if len(parts) >= 4 and re.fullmatch(r'[A-Za-z0-9_.@:-]+\.service', parts[0]):
            units.append({'unit': parts[0], 'active': parts[2], 'sub': parts[3],
                          'pid': 0, 'type': ''})
    if not units:
        return [], ''
    # Batch property queries; retain the listing even when a unit disappears mid-run.
    by_name = {entry['unit']: entry for entry in units}
    for offset in range(0, len(units), 100):
        names = [u['unit'] for u in units[offset:offset + 100]]
        args = ['systemctl', 'show', '--no-pager', '--property=Id,MainPID,Type,ActiveState,SubState'] + names
        try:
            result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                                    text=True, timeout=15, check=False,
                                    env=dict(os.environ, LC_ALL='C', SYSTEMD_COLORS='0'))
        except (OSError, subprocess.TimeoutExpired):
            continue
        for block in re.split(r'\n\s*\n', result.stdout.strip()):
            props = dict(line.split('=', 1) for line in block.splitlines() if '=' in line)
            entry = by_name.get(props.get('Id'))
            if entry:
                entry['pid'] = int(props.get('MainPID', '0')) if props.get('MainPID', '').isdigit() else 0
                entry['type'] = props.get('Type', '')
                entry['active'] = props.get('ActiveState', entry['active'])
                entry['sub'] = props.get('SubState', entry['sub'])
    return units, ''


def process_record(pid, proc='/proc'):
    path = Path(proc) / str(pid)
    comm = read_text(path / 'comm')
    try:
        argv = [x.decode('utf-8', 'replace') for x in (path / 'cmdline').read_bytes().split(b'\0') if x]
    except OSError:
        argv = []
    return {'pid': pid, 'comm': comm, 'argv': argv}


def socket_inventory(proc='/proc'):
    """Inventory local TCP listeners and bound UDP sockets, with best-effort owners."""
    listeners = []
    for proto in ('tcp', 'tcp6', 'udp', 'udp6'):
        for line in read_text(Path(proc) / 'net' / proto).splitlines()[1:]:
            fields = line.split()
            if len(fields) < 10 or (proto.startswith('tcp') and fields[3] != '0A'):
                continue
            # UDP state 07 means unconnected: these are bound sockets, not guaranteed servers.
            if proto.startswith('udp') and fields[3] != '07':
                continue
            try:
                addr, port = fields[1].split(':')
                raw = bytes.fromhex(addr)
                if len(raw) == 4:
                    ip = ipaddress.ip_address(raw[::-1])
                elif len(raw) == 16:
                    ip = ipaddress.ip_address(b''.join(raw[i:i+4][::-1] for i in range(0, 16, 4)))
                else:
                    continue
                port = int(port, 16)
                if not port:
                    continue
                listeners.append({'protocol': proto, 'address': str(ip), 'port': port,
                                  'scope': 'loopback' if ip.is_loopback else 'non-loopback/wildcard',
                                  'inode': fields[9], 'owners': []})
            except (ValueError, IndexError):
                continue
    inodes = {entry['inode'] for entry in listeners}
    owners = {}
    try:
        processes_dirs = list(Path(proc).iterdir())
    except OSError:
        processes_dirs = []
    denied = 0
    for procdir in processes_dirs:
        if not procdir.name.isdigit():
            continue
        try:
            fds = list((procdir / 'fd').iterdir())
        except (PermissionError, OSError):
            denied += 1
            continue
        found = set()
        for fd in fds:
            try:
                link = os.readlink(fd)
            except OSError:
                continue
            match = re.fullmatch(r'socket:\[(\d+)\]', link)
            if match and match.group(1) in inodes:
                found.add(match.group(1))
        if found:
            cg = read_text(procdir / 'cgroup')
            unit_match = re.search(r'(?:^|/)([A-Za-z0-9_.@:-]+\.service)(?:/|$)', cg, re.M)
            owner = {'pid': int(procdir.name), 'comm': read_text(procdir / 'comm'),
                     'unit': unit_match.group(1) if unit_match else ''}
            for inode in found:
                owners.setdefault(inode, []).append(owner)
    for entry in listeners:
        entry['owners'] = owners.get(entry['inode'], [])
    return sorted(listeners, key=lambda x: (x['protocol'], x['port'], x['address'])), denied


def selector_for_process(record):
    """Select a stable, non-secret command fragment for shared interpreters."""
    comm, argv = record['comm'], record['argv']
    if not comm or not SAFE_NAME.fullmatch(comm) or not argv:
        return None
    generic = re.fullmatch(r'(?:python[0-9.]*|java|ruby|node|nodejs|bash|sh|dash|agent)', comm)
    if not generic:
        return '-C ' + shlex.quote(comm)
    safe_token = re.compile(r'^[A-Za-z0-9_./+@=-]+$')
    if comm == 'java':
        for i, arg in enumerate(argv):
            if arg == '-jar' and i + 1 < len(argv) and safe_token.fullmatch(argv[i + 1]):
                return '-C java -a ' + shlex.quote(argv[i + 1])
            if re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*(?:\.[A-Za-z_][A-Za-z0-9_]*){2,}', arg):
                return '-C java -a ' + shlex.quote(arg)
    else:
        for index, arg in enumerate(argv[1:], 1):
            if argv[index - 1] == '-m' and re.fullmatch(r'[A-Za-z_][A-Za-z0-9_.]*', arg):
                return '-C ' + shlex.quote(comm) + ' -a ' + shlex.quote(arg)
            if arg.startswith('/') and safe_token.fullmatch(arg):
                return '-C ' + shlex.quote(comm) + ' -a ' + shlex.quote(arg)
        if comm == 'agent' and argv[0].startswith('/') and safe_token.fullmatch(argv[0]):
            return '-C agent -a ' + shlex.quote(argv[0])
    return None


def runtime_candidates(procs):
    """Discover additional Java, Node and Python-module processes without logging argv."""
    candidates = set()
    for comm, cmd in procs:
        if not re.fullmatch(r'(?:java|node|nodejs|python[0-9.]*)', comm):
            continue
        try:
            argv = shlex.split(cmd)
        except ValueError:
            continue
        if comm.startswith('python') and '-m' not in argv:
            continue
        selector = selector_for_process({'comm': comm, 'argv': argv})
        if selector:
            candidates.add((comm, selector))
    return sorted(candidates)


def generate(plugin_dir, procs=None, mounts=None, ports=None, units=None, sockets=None, proc_root='/proc'):
    available = plugins(plugin_dir)
    if not available:
        raise ValueError('No executable plugins found in ' + plugin_dir)
    procs = processes() if procs is None else procs
    mounts = mountpoints() if mounts is None else mounts
    ports = local_listening_ports() if ports is None else ports
    detected = detected_services(procs)
    units = [] if units is None else units
    sockets = [] if sockets is None else sockets
    rows, notices = [], []
    used = set()

    def add(name, desc, command, interval, cls):
        if name in used:
            raise ValueError('Duplicate check name: ' + name)
        line = '[{}] {};{};1;{} {}'.format(name, desc, command, interval,
                                             json.dumps({'class': cls}, separators=(',', ':')))
        if len(line.encode('utf-8')) >= 1023:
            notices.append('Skipped an overly long check: ' + name)
            return
        rows.append(line)
        used.add(name)

    # These interfaces are conventional or based on the supplied examples.
    # Local versions and thresholds should be reviewed before deployment.
    if 'check_icmp' in available:
        add('check_ping', 'Local ICMP', 'check_icmp -H 127.0.0.1 -w 100,20% -c 500,60%', 1, 'network')
    if 'check_disk' in available:
        for mount in mounts:
            if not MOUNT_SAFE.fullmatch(mount):
                notices.append('Skipped unsupported mount path: ' + repr(mount))
                continue
            suffix = 'root' if mount == '/' else re.sub(r'[^A-Za-z0-9]+', '_', mount.strip('/')).strip('_')
            name = 'check_disk_' + suffix
            if name in used:  # do not silently conflate mount points
                notices.append('Skipped mount with duplicate check name: ' + mount)
                continue
            add(name, 'Disk ' + mount, 'check_disk -w 10% -c 5% -p ' + shlex.quote(mount), 5, 'infra')
    else:
        notices.append('check_disk is unavailable; filesystem checks were not generated')
    if 'check_load' in available:
        cpus = os.cpu_count() or 1
        warn, crit = max(1, cpus * .7), max(2, cpus * .9)
        add('check_load', 'System load', 'check_load -w {0:.1f},{0:.1f},{0:.1f} -c {1:.1f},{1:.1f},{1:.1f}'.format(warn, crit), 1, 'system')
    for filename, name, desc, cmd, interval in [
        ('check_memory.py', 'check_memory', 'Memory usage', 'check_memory.py -w 85 -c 90', 1),
        ('check_cpu_stats.sh', 'check_cpu_stats', 'CPU statistics', 'check_cpu_stats.sh -w 95,95,60 -c 100,100,100 -i 1 -n 3', 2),
        ('check_uptime.sh', 'check_uptime', 'Uptime', 'check_uptime.sh 30 60', 60),
        ('check_users', 'check_users', 'Logged-in users', 'check_users -w 75 -c 100', 5),
        ('check_open_files.pl', 'check_open_files', 'Open files', 'check_open_files.pl -w 30 -c 50', 5),
        ('check_procs', 'check_all_procs', 'All processes', 'check_procs', 5),
    ]:
        if filename in available:
            add(name, desc, cmd, interval, 'system')
    http_ports = set()
    if 'nginx' in detected or 'apache' in detected:
        http_ports.update({80, 8080, 443, 8443} & ports)
    if 'gunicorn' in detected:
        http_ports.update(gunicorn_bind_ports(detected['gunicorn']) & ports)
    if 'check_http' in available:
        for port in sorted(http_ports):
            tls = ' -S' if port in (443, 8443) else ''
            add('check_http_' + str(port), 'Local HTTP port ' + str(port),
                'check_http -H localhost -I 127.0.0.1 -p ' + str(port) + tls, 2, 'application')
    elif http_ports:
        notices.append('Web server detected but check_http is unavailable; no HTTP response check generated')
    for service, matches in detected.items():
        specialty = [p for p in SERVICES[service][1] if p in available]
        if specialty:
            notices.append('{}: matching plugin(s) {}. Configure and verify arguments/credentials for a service-level check.'.format(service, ', '.join(specialty)))
        if 'check_procs' not in available:
            notices.append(service + ': check_procs unavailable; no automatic service check generated')
            continue
        for proc_name in sorted({x[0] for x in matches}):
            extra = ''
            if service in ('supervisord', 'gunicorn') and proc_name.startswith('python'):
                extra = ' -a ' + service
            if service in CMD_REQUIRED:
                candidates = [needle for needle in CMD_REQUIRED[service]
                              if any(needle in cmd.lower() for name, cmd in matches if name == proc_name)]
                if not candidates:
                    continue
                needle = candidates[0]
                sample = next(cmd for name, cmd in matches if name == proc_name and needle in cmd.lower())
                index = sample.lower().index(needle)
                extra = ' -a ' + shlex.quote(sample[index:index + len(needle)])
            add('check_proc_' + service + ('_' + proc_name if len({x[0] for x in matches}) > 1 else ''),
                'Process ' + service + ' (' + proc_name + ')',
                'check_procs -C ' + proc_name + extra + ' -c 1:', 1, 'application')
    for proc_name, script in python_apps(procs):
        if 'check_procs' not in available:
            notices.append('Python application detected: ' + script + '; check_procs unavailable')
            continue
        # Stable suffix distinguishes paths that sanitize to the same check name.
        suffix = re.sub(r'[^A-Za-z0-9]+', '_', script.strip('/')).strip('_')[:48]
        digest = hashlib.sha256((proc_name + '\0' + script).encode()).hexdigest()[:8]
        add('check_app_' + suffix + '_' + digest, 'Python application ' + script,
            'check_procs -C ' + proc_name + ' -a ' + shlex.quote(script) + ' -c 1:', 1, 'application')
    # Oracle PMON names contain the instance identifier; discover each separately.
    oracle_instances = set()
    for _comm, cmd in procs:
        for match in re.finditer(r'(?:^|\s)(ora_pmon_[A-Za-z0-9_$#]+)(?=\s|$)', cmd, re.I):
            oracle_instances.add(match.group(1))
    if oracle_instances:
        found_plugins = [name for name in ('check_oracle', 'check_oracle_health', 'check_oracle_health.pl') if name in available]
        notices.append('Oracle instances: {}; matching plugins: {}. Database-level checks require configuration.'.format(
            ', '.join(sorted(oracle_instances)), ', '.join(found_plugins) or 'none'))
        if 'check_procs' in available:
            for instance in sorted(oracle_instances):
                suffix = re.sub(r'[^A-Za-z0-9_]+', '_', instance)
                digest = hashlib.sha256(instance.encode()).hexdigest()[:8]
                add('check_oracle_' + suffix[:40] + '_' + digest, 'Oracle instance ' + suffix,
                    'check_procs -a ' + shlex.quote(instance) + ' -c 1:', 1, 'application')
    # A running systemd unit is useful evidence even without a product-specific rule.
    already_covered = {(name, cmd) for matches in detected.values() for name, cmd in matches}
    for unit in units:
        if unit.get('active') != 'active' or unit.get('sub') != 'running' or not unit.get('pid'):
            continue
        if unit.get('type') == 'oneshot':
            continue
        stem = unit['unit'].removesuffix('.service') if hasattr(str, 'removesuffix') else unit['unit'][:-8]
        stem = stem.split('@', 1)[0].lower().replace('-', '_')
        specific = [name for name in available if re.sub(r'\.(?:py|pl|sh)$', '', name.lower()).replace('-', '_') == 'check_' + stem]
        if specific:
            notices.append('systemd {}: matching plugins {}; verify their arguments and credentials'.format(unit['unit'], ', '.join(sorted(specific))))
        record = process_record(unit['pid'], proc_root)
        cmdline = ' '.join(record['argv'])
        if (record['comm'], cmdline) in already_covered:
            continue
        # PMON checks above are more specific than the associated generic service.
        if any(instance in cmdline for instance in oracle_instances):
            continue
        selector = selector_for_process(record)
        if not selector or 'check_procs' not in available:
            notices.append('systemd {} is running; a safe process selector or check_procs is unavailable'.format(unit['unit']))
            continue
        clean = re.sub(r'[^A-Za-z0-9_]+', '_', unit['unit']).strip('_')[:45]
        digest = hashlib.sha256(unit['unit'].encode()).hexdigest()[:8]
        add('check_unit_' + clean + '_' + digest, 'Systemd service ' + unit['unit'],
            'check_procs ' + selector + ' -c 1:', 1, 'application')
    # TCP checks are only created when an active systemd service owns the socket.
    active_units = {u['unit'] for u in units if u.get('active') == 'active' and u.get('sub') == 'running'}
    endpoints = set()
    for entry in sockets:
        if entry['protocol'] != 'tcp' or not any(o.get('unit') in active_units for o in entry['owners']):
            continue
        host = '127.0.0.1' if entry['address'] == '0.0.0.0' else entry['address']
        endpoints.add((host, entry['port']))
    if endpoints and 'check_tcp' not in available:
        notices.append('TCP listeners found; check_tcp is unavailable')
    for host, port in sorted(endpoints)[:64]:
        if 'check_tcp' not in available:
            break
        suffix = hashlib.sha256((host + ':' + str(port)).encode()).hexdigest()[:8]
        add('check_tcp_{}_{}'.format(port, suffix), 'Local TCP {} port {}'.format(host, port),
            'check_tcp -H {} -p {}'.format(host, port), 2, 'network')
    if len(endpoints) > 64:
        notices.append('Only 64 TCP endpoints were selected for checks; remaining listeners are inventory only')
    lines = [HEADER, '# Generated by almond_discovery_agent.py; review thresholds and plugin arguments.',
             '# Intervals are in minutes. Service-specific plugin suggestions are listed below.']
    lines += ['# ' + note for note in notices]
    lines += rows
    return '\n'.join(lines) + '\n', detected, notices


def test_check(plugin_dir, command, timeout):
    """Run exactly the generated argument vector, without a shell, in caller's cwd."""
    try:
        args = shlex.split(command)
        if not args or not SAFE_NAME.fullmatch(args[0]):
            return False, 'INVALID', 'Invalid plugin name'
        args[0] = str((Path(plugin_dir) / args[0]).absolute())
        with tempfile.TemporaryFile() as output:
            proc = subprocess.Popen(args, stdin=subprocess.DEVNULL, stdout=output,
                                    stderr=subprocess.STDOUT, start_new_session=True)
            try:
                code = proc.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                try:
                    os.killpg(proc.pid, signal.SIGKILL)
                except ProcessLookupError:
                    pass
                proc.wait()
                return False, 'TIMEOUT', 'Exceeded {} seconds'.format(timeout)
            output.seek(0)
            data = output.read(65536).decode('utf-8', 'replace')
        lines = [line.strip() for line in data.splitlines() if line.strip()]
        message = lines[-1][:300] if lines else '(no output)'
        if not lines:
            return False, 'INVALID', message
        if re.search(r'(?im)^\s*usage\s*:|(?:invalid|unknown|unrecognized|unrecognised) (?:command.line )?(?:option|argument)', data):
            return False, 'INVALID', message
        if code == 0:
            return True, 'OK', message
        if code == 1:
            return True, 'WARNING', message
        if code == 2:
            return True, 'CRITICAL', message
        if code == 3:
            return False, 'UNKNOWN', message
        return False, 'ERROR', 'Exit {}: {}'.format(code, message)
    except (OSError, ValueError) as exc:
        return False, 'ERROR', str(exc)


def probe_plugin_help(plugin_dir, name, timeout):
    """Probe each generated plugin with -h / --help; never interpret arbitrary text as code."""
    if not SAFE_NAME.fullmatch(name):
        return None, 'unsafe plugin name'
    program = str((Path(plugin_dir) / name).absolute())
    reasons = []
    for flag in ('-h', '--help'):
        try:
            with tempfile.TemporaryFile() as output:
                proc = subprocess.Popen([program, flag], stdin=subprocess.DEVNULL,
                                        stdout=output, stderr=subprocess.STDOUT,
                                        start_new_session=True)
                try:
                    proc.wait(timeout=timeout)
                except subprocess.TimeoutExpired:
                    try:
                        os.killpg(proc.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                    proc.wait()
                    reasons.append(flag + ' timed out')
                    continue
                output.seek(0)
                data = output.read(65536).decode('utf-8', 'replace')
        except OSError as exc:
            return None, str(exc)
        data = re.sub(r'\x1b\[[0-9;]*[A-Za-z]', '', data)
        # Some plugins run a normal check on unknown options. Only accept actual help-like text.
        if re.search(r'(?im)^\s*(?:usage\s*:|options\s*:|(?:-h|--help)(?:[\s,=]|$)|(?:-w|--warning)(?:[\s,=]|$))', data):
            return data, flag
        reasons.append(flag + ' did not return recognizable help')
    return None, '; '.join(reasons)


def help_option_text(data):
    """Extract short option descriptions and indented continuation lines."""
    result = {'warning': [], 'critical': []}
    current = None
    for line in data.splitlines():
        match = re.match(r'^\s*(?:-([wc])\b|--(warning|critical)\b)', line, re.I)
        if match:
            short, longname = match.groups()
            current = 'warning' if (short or '').lower() == 'w' or (longname or '').lower() == 'warning' else 'critical'
            result[current].append(line.strip())
        elif current and line[:1].isspace() and line.strip() and not re.match(r'^\s*-\w', line):
            result[current].append(line.strip())
        else:
            current = None
    return {key: ' '.join(lines) for key, lines in result.items()}


def memory_help_direction(help_text):
    """Infer a direction only when both option descriptions state it explicitly."""
    options = help_option_text(help_text)
    directions = []
    for name in ('warning', 'critical'):
        text = options[name]
        above = bool(re.search(r'\b(?:above|greater than|more than|exceeds?|over)\b', text, re.I))
        below = bool(re.search(r'\b(?:below|less than|under)\b', text, re.I))
        if above == below:
            return None
        directions.append('above' if above else 'below')
    if directions[0] != directions[1]:
        return None
    return directions[0]


def memory_help_conflict(command, help_text):
    """Flag threshold order only when both option descriptions state the same direction."""
    direction = memory_help_direction(help_text)
    if direction is None:
        return None
    try:
        args = shlex.split(command)
        warning = float(args[args.index('-w') + 1])
        critical = float(args[args.index('-c') + 1])
    except (ValueError, IndexError):
        return None
    if direction == 'above' and warning >= critical:
        return 'help says thresholds trigger above values, but warning is not below critical'
    if direction == 'below' and warning <= critical:
        return 'help says thresholds trigger below values, but warning is not above critical'
    return None


def memory_help_defaults(help_text):
    """Read explicit percentage defaults from the two option descriptions."""
    options = help_option_text(help_text)
    result = {}
    for name in ('warning', 'critical'):
        description = options[name]
        if not re.search(r'\bpercent(?:age)?\b|%', description, re.I):
            return None
        match = re.search(
            r'\bdefault(?:\s+value)?(?:\s+(?:is\s+)?(?:set\s+)?(?:to|at))?\s*[:=]?\s*(\d+(?:\.\d+)?)\s*%?',
            description, re.I)
        if not match:
            return None
        value = float(match.group(1))
        if not 0 <= value <= 100:
            return None
        result[name] = value
    if result['warning'] == result['critical']:
        return None
    direction = memory_help_direction(help_text)
    if direction == 'above' and result['warning'] >= result['critical']:
        return None
    if direction == 'below' and result['warning'] <= result['critical']:
        return None
    return result


def apply_memory_help_defaults(command, defaults):
    """Use documented defaults only for the generated memory check."""
    try:
        args = shlex.split(command)
        if not args or args[0] != 'check_memory.py':
            return command
        warn_index, crit_index = args.index('-w') + 1, args.index('-c') + 1
        args[warn_index] = format(defaults['warning'], 'g')
        args[crit_index] = format(defaults['critical'], 'g')
        return ' '.join(shlex.quote(arg) for arg in args)
    except (ValueError, IndexError, KeyError):
        return command


def inspect_plugin_help(conf, plugin_dir, timeout):
    """Inspect each executable once; return adjusted config and argument conflicts."""
    cache = {}
    conflicts = {}
    output = []
    for line in conf.splitlines():
        if not line.startswith('['):
            output.append(line)
            continue
        parts = line.split(';', 3)
        name = parts[0].split(']', 1)[0][1:]
        try:
            argv = shlex.split(parts[1])
        except (ValueError, IndexError):
            output.append(line)
            continue
        if not argv:
            output.append(line)
            continue
        plugin = argv[0]
        if plugin not in cache:
            data, detail = probe_plugin_help(plugin_dir, plugin, timeout)
            cache[plugin] = data
            if data is None:
                print('HELP {}: unavailable ({})'.format(plugin, detail), file=sys.stderr, flush=True)
            else:
                options = help_option_text(data)
                print('HELP {}: obtained with {}'.format(plugin, detail), file=sys.stderr, flush=True)
                for kind in ('warning', 'critical'):
                    if options[kind]:
                        snippet = ''.join(c if c.isprintable() else ' ' for c in options[kind])[:350]
                        print('  {}: {}'.format(kind, snippet), file=sys.stderr, flush=True)
                if not any(options.values()):
                    print('  Threshold meanings not identified; review plugin help manually.', file=sys.stderr, flush=True)
        if plugin == 'check_memory.py' and cache[plugin] is not None:
            direction = memory_help_direction(cache[plugin])
            print('HELP {}: threshold direction {}'.format(
                name, direction if direction else 'not determined; review manually'), file=sys.stderr, flush=True)
            defaults = memory_help_defaults(cache[plugin])
            if defaults:
                parts[1] = apply_memory_help_defaults(parts[1], defaults)
                output.append('# Help: using documented percentage defaults for memory thresholds')
                print('HELP {}: documented defaults warning={:g}% critical={:g}%'.format(
                    name, defaults['warning'], defaults['critical']), file=sys.stderr, flush=True)
            problem = memory_help_conflict(parts[1], cache[plugin])
            if problem:
                conflicts[name] = problem
        output.append(';'.join(parts))
    return '\n'.join(output) + '\n', conflicts


def test_configuration(conf, plugin_dir, timeout, help_conflicts=None):
    """Disable checks that do not produce a usable monitoring result."""
    output = []
    help_conflicts = help_conflicts or {}
    counts = {'OK': 0, 'WARNING': 0, 'CRITICAL': 0, 'DISABLED': 0}
    for line in conf.splitlines():
        if not line.startswith('['):
            output.append(line)
            continue
        parts = line.split(';', 3)
        name = parts[0].split(']', 1)[0][1:]
        valid, status, message = test_check(plugin_dir, parts[1], timeout)
        message = ''.join(c if c.isprintable() else ' ' for c in message)
        print('TEST {}: {} - {}'.format(name, status, message), file=sys.stderr, flush=True)
        if name in help_conflicts:
            print('HELP CONFLICT {}: {}'.format(name, help_conflicts[name]), file=sys.stderr, flush=True)
            valid, status = False, 'HELP_CONFLICT'
        if not valid:
            parts[2] = '0'
            counts['DISABLED'] += 1
        else:
            counts[status] += 1
        output.append('# Test: {}{}'.format(status, '' if valid else ' (disabled; investigate)'))
        output.append(';'.join(parts))
    return '\n'.join(output) + '\n', counts


def print_inventory(units, systemd_error, sockets, denied, procs, out):
    print('Environment: ' + environment(), file=out)
    services = detected_services(procs)
    print('Recognized applications: ' + (', '.join(sorted(services)) or 'none'), file=out)
    oracle = sorted({m.group(1) for _comm, cmd in procs for m in
                     re.finditer(r'(?:^|\s)(ora_pmon_[A-Za-z0-9_$#]+)(?=\s|$)', cmd, re.I)})
    if oracle:
        print('Oracle PMON instances: ' + ', '.join(oracle), file=out)
    for _comm, script in python_apps(procs):
        print('Python application: ' + script, file=out)
    for runtime, selector in runtime_candidates(procs):
        print('Runtime candidate ({}): {}'.format(runtime, selector), file=out)
    if systemd_error:
        print('Systemd: ' + systemd_error, file=out)
    else:
        print('Systemd services (running and failed):', file=out)
        for unit in sorted(units, key=lambda u: u['unit']):
            print('  {}: {}/{}{}'.format(unit['unit'], unit['active'], unit['sub'],
                  ' pid={}'.format(unit['pid']) if unit['pid'] else ''), file=out)
    print('Local sockets (non-loopback binding does not prove external reachability):', file=out)
    hints = {1521: 'possible Oracle listener', 5000: 'possible Sybase or another service',
             5432: 'possible PostgreSQL', 9092: 'possible Kafka/Redpanda',
             9000: 'possible MinIO or another service'}
    for item in sockets:
        owners = sorted({(o['unit'] or o['comm'] or 'unknown') for o in item['owners']})
        hint = hints.get(item['port'], '')
        print('  {} {}:{} [{}] owner={}{}'.format(
            item['protocol'], item['address'], item['port'], item['scope'],
            ','.join(owners) or 'unknown', '; hint=' + hint if hint else ''), file=out)
    print('UDP entries are bound sockets and may include clients.', file=out)
    if denied:
        print('Could not inspect file descriptors for {} processes; socket owners may be incomplete.'.format(denied), file=out)
    print('', file=out, flush=True)


def write_config(target, content, force):
    target = Path(target)
    if not target.parent.is_dir():
        raise ValueError('Target directory does not exist: ' + str(target.parent))
    if target.is_symlink():
        raise ValueError('Refusing to replace a symbolic link: ' + str(target))
    # Stage in the same directory; use a hard link for exclusive creation when new.
    fd, temp_name = tempfile.mkstemp(prefix='.almond-discovery-', dir=str(target.parent))
    try:
        os.fchmod(fd, 0o644)
        with os.fdopen(fd, 'w', encoding='utf-8') as f:
            f.write(content)
            f.flush()
            os.fsync(f.fileno())
        if target.exists():
            if not force:
                raise ValueError('Target already exists; review output and use --force to replace it with a backup')
            if not target.is_file():
                raise ValueError('Target is not a regular file: ' + str(target))
            stamp = datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f')
            backup = target.with_name(target.name + '.bak-' + stamp)
            backup_fd = os.open(str(backup), os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
            with target.open('rb') as src, os.fdopen(backup_fd, 'wb') as dst:
                shutil.copyfileobj(src, dst)
                dst.flush()
                os.fsync(dst.fileno())
            shutil.copystat(target, backup)
            # Preserve existing permissions and ownership on replacement.
            st = target.stat()
            os.chmod(temp_name, st.st_mode & 0o7777)
            os.chown(temp_name, st.st_uid, st.st_gid)
            os.replace(temp_name, target)
            print('Backup: ' + str(backup), file=sys.stderr)
        else:
            try:
                os.link(temp_name, target)  # fails if another writer created the target
            except FileExistsError:
                raise ValueError('Target was created concurrently; rerun after reviewing it')
    finally:
        try:
            os.unlink(temp_name)
        except FileNotFoundError:
            pass


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--plugins-dir', default='/opt/almond/plugins', help='Plugin directory (default: %(default)s)')
    parser.add_argument('--output', default='/etc/almond/plugins.conf', help='Target path for --apply (default: %(default)s)')
    parser.add_argument('--apply', action='store_true', help='Write configuration instead of printing it')
    parser.add_argument('--force', action='store_true', help='Replace existing file after making a backup (requires --apply)')
    parser.add_argument('--inventory-only', action='store_true', help='Show systemd services and sockets without executing plugins')
    parser.add_argument('--test-timeout', type=float, default=15, help='Seconds allowed for each plugin test (default: %(default)s)')
    parser.add_argument('--help-timeout', type=float, default=3, help='Seconds allowed per help attempt (default: %(default)s)')
    parser.add_argument('--skip-plugin-help', action='store_true', help='Do not execute plugins with -h or --help')
    args = parser.parse_args()
    if args.force and not args.apply:
        parser.error('--force requires --apply')
    if args.inventory_only and args.apply:
        parser.error('--inventory-only cannot be combined with --apply')
    if not 0 < args.test_timeout <= 3600:
        parser.error('--test-timeout must be greater than 0 and no more than 3600')
    if not 0 < args.help_timeout <= 3600:
        parser.error('--help-timeout must be greater than 0 and no more than 3600')
    try:
        units, systemd_error = systemd_inventory()
        sockets, denied = socket_inventory()
        procs = processes()
        print_inventory(units, systemd_error, sockets, denied, procs, sys.stdout if args.inventory_only else sys.stderr)
        if args.inventory_only:
            return 0
        conf, found, notes = generate(args.plugins_dir, procs=procs, units=units, sockets=sockets)
        if not any(line.startswith('[') for line in conf.splitlines()):
            raise ValueError('No checks could be generated; refusing to write an empty configuration')
        print('Detected services: ' + (', '.join(sorted(found)) or 'none'), file=sys.stderr)
        conflicts = {}
        if not args.skip_plugin_help:
            print('Inspecting plugin help (per-attempt timeout {}s)...'.format(args.help_timeout), file=sys.stderr, flush=True)
            conf, conflicts = inspect_plugin_help(conf, args.plugins_dir, args.help_timeout)
        print('Testing plugins as user ID {} (per-check timeout {}s)...'.format(os.geteuid(), args.test_timeout), file=sys.stderr, flush=True)
        conf, counts = test_configuration(conf, args.plugins_dir, args.test_timeout, conflicts)
        print('Test summary: ' + ', '.join('{}={}'.format(k, v) for k, v in counts.items()), file=sys.stderr)
        print('Review thresholds, test results, and plugin suggestions before using them.', file=sys.stderr)
        if args.apply:
            if sum(counts[k] for k in ('OK', 'WARNING', 'CRITICAL')) == 0:
                sys.stdout.write(conf)
                raise ValueError('No usable checks; refusing to write configuration')
            write_config(args.output, conf, args.force)
            print('Wrote: ' + args.output, file=sys.stderr)
        else:
            sys.stdout.write(conf)
        return 0
    except (OSError, ValueError) as exc:
        print('Error: ' + str(exc), file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
