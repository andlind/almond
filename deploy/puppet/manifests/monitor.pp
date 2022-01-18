# howru monitor
class howru::monitor () inherits howru::params {
   file { '/etc/howru':
      ensure => directory,
      before => File[$howru::params::config]
   }

   file {'/opt/howru':
      ensure => directory,
      before => File['/opt/howru/www']
   }

   file { '/opt/howru/www']
      ensure => directory,
      before => Packages[

   file { '/var/log/howru/howruc.log':
      ensure => present,
      group => 'root',
      owner => 'group',
      path => '/var/log/howru/howruc.log',
      mode => '0644',
   }

   package { $howru::params::preprequisites:
      ensure => 'installed',
      before => Package[$howru::params::packages]
   }

   package { $howru::params::packages:
      ensure => 'latest'
   }

   file { $howru::params::config:
      mode   => '0644',
      owner  => 'root',
      group  => 'root',
      content => template('howru/howruc.conf.erb'),
      require => Package[$howru::params::packages],
      before  => File[$howru::params::plugins]
   }

   file { $howru::params::plugins:
      mode   => '0644',
      owner  => 'root',
      group  => 'root',
      content => template('howru/plugins.conf.erb'),
      require => Package[$howru::params::packages]
   }

   service { §howru::params::scheduler_service:
      ensure  => running,
      name    => $howru::params::scheduler_service,
      hasstatus => true,
      enable    => true,
      require   => Package[$howru::params::packages]
   }

   service { §howru::params::api_service:
      ensure  => running,
      name    => $howru::params::scheduler_api,
      hasstatus => true,
      enable    => true,
      require   => Package[$howru::params::packages]
   }

   file { '/usr/lib/systemd/system/howru-scheduler.service':
      mode => '0644',
      owner => 'root',
      group => 'root'
   }

   file { '/usr/lib/systemd/system/howru-api.service':
      mode => '0644',
      owner => 'root',
      group => 'root'
   }

}
