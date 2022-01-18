# HowRu params
class howru::params {
  $config            = '/etc/howru/howruc.cfg'
  $packages          = ['howru-monitor']
  $scheduler_service = 'howru-scheduler.service'
  $api_service       = 'howru-api.service'
  $config            = '/etc/howru/howruc.conf'
  $plugins           = '/etc/howru/plugins.conf'

  case $::operatingsystem {
    'redhat', 'centos', 'OracleLinux':  {
      if ($::operatingsystemmajrelease == '6') {
        $libdir          = '/usr/lib64/nagios/plugins'
      }
      else {
        # Add path to op5servers
        if (('op5' in $::hostname) or ('op0' in $::hostname)) {
           $libdir          = '/opt/plugins/custom'
        }
        else {
           $libdir          = '/usr/local/nagios/libexec'
        }
      }
      $user            = 'howru'
      $group           = 'howru'
      if ($::operatingsystemmajrelease == '6') {
         $client_packages = ['op5-svs-plugins','python-psutil','perl-Sys-Statistics-Linux','ksh']
      }
      else {
         $client_packages = ['op5-svs-plugins','python-psutil','perl-Sys-Statistics-Linux','ksh','python3-nagiosplugin']
      }
    }
    'Ubuntu':  {
      $libdir          = '/usr/local/nagios/libexec'
      $user            = 'nagios'
      $group           = 'nagios'
      $prepackages     = ['libsys-statistics-linux-perl','ksh']
    }
    default: { notify('Unrecognized operating system.') }
  }

}

