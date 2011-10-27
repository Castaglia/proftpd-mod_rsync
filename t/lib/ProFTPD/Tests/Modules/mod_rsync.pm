package ProFTPD::Tests::Modules::mod_rsync;

use lib qw(t/lib);
use base qw(Test::Unit::TestCase ProFTPD::TestSuite::Child);
use strict;

use File::Copy;
use File::Path qw(mkpath rmtree);
use File::Spec;
use IO::Handle;

use ProFTPD::TestSuite::FTP;
use ProFTPD::TestSuite::Utils qw(:auth :config :features :running :test :testsuite);

$| = 1;

my $order = 0;

my $TESTS = {
  rsync_list_empty_dir => {
    order => ++$order,
    test_class => [qw(forking mod_sftp)],
  },

  rsync_list_file_cvs_exclude => {
    order => ++$order,
    test_class => [qw(forking mod_sftp)],
  },

  rsync_list_file_numeric_ids => {
    order => ++$order,
    test_class => [qw(forking mod_sftp)],
  },

};

sub new {
  return shift()->SUPER::new(@_);
}

sub list_tests {
  # Check for the required Perl modules:
  #
  #  File-RsyncP

  my $required = [qw(
    File::RsyncP
  )];

  foreach my $req (@$required) {
    eval "use $req";
    if ($@) {
      print STDERR "\nWARNING:\n + Module '$req' not found, skipping all tests\n";

      if ($ENV{TEST_VERBOSE}) {
        print STDERR "Unable to load $req: $@\n";
      }

      return qw(testsuite_empty_test);
    }
  }

#  return testsuite_get_runnable_tests($TESTS);
  return qw(
    rsync_list_empty_dir
  );
}

sub set_up {
  my $self = shift;
  $self->{tmpdir} = testsuite_get_tmp_dir();
  
  # Create temporary scratch dir
  eval { mkpath($self->{tmpdir}) };
  if ($@) {
    my $abs_path = File::Spec->rel2abs($self->{tmpdir});
    die("Can't create dir $abs_path: $@");
  }

  if (feature_have_module_compiled('mod_sftp.c')) {
    # Make sure that mod_sftp does not complain about permissions on the hostkey
    # files.

    my $rsa_host_key = File::Spec->rel2abs('t/etc/modules/mod_sftp/ssh_host_rsa_key');
    my $dsa_host_key = File::Spec->rel2abs('t/etc/modules/mod_sftp/ssh_host_dsa_key');

    unless (chmod(0400, $rsa_host_key, $dsa_host_key)) {
      die("Can't set perms on $rsa_host_key, $dsa_host_key: $!");
    }
  }
}

sub tear_down {
  my $self = shift;

  # Remove temporary scratch dir
  if ($self->{tmpdir}) {
    eval { rmtree($self->{tmpdir}) };
  }

  undef $self;
}

sub rsync_list_empty_dir {
  my $self = shift;
  my $tmpdir = $self->{tmpdir};

  my $config_file = "$tmpdir/rsync.conf";
  my $pid_file = File::Spec->rel2abs("$tmpdir/rsync.pid");
  my $scoreboard_file = File::Spec->rel2abs("$tmpdir/rsync.scoreboard");

  my $log_file = File::Spec->rel2abs('tests.log');

  my $auth_user_file = File::Spec->rel2abs("$tmpdir/rsync.passwd");
  my $auth_group_file = File::Spec->rel2abs("$tmpdir/rsync.group");

  my $user = 'proftpd';
  my $passwd = 'test';
  my $home_dir = File::Spec->rel2abs($tmpdir);
  my $uid = 500;
  my $gid = 500;

  # Make sure that, if we're running as root, that the home directory has
  # permissions/privs set for the account we create
  if ($< == 0) {
    unless (chmod(0755, $home_dir)) {
      die("Can't set perms on $home_dir to 0755: $!");
    }

    unless (chown($uid, $gid, $home_dir)) {
      die("Can't set owner of $home_dir to $uid/$gid: $!");
    }
  }

  auth_user_write($auth_user_file, $user, $passwd, $uid, $gid, $home_dir,
    '/bin/bash');
  auth_group_write($auth_group_file, 'ftpd', $gid, $user);

  my $timeout_idle = 15;

  my $rsa_host_key = File::Spec->rel2abs('t/etc/modules/mod_sftp/ssh_host_rsa_key');
  my $dsa_host_key = File::Spec->rel2abs('t/etc/modules/mod_sftp/ssh_host_dsa_key');

  my $rsa_priv_key = File::Spec->rel2abs('t/etc/modules/mod_sftp/test_rsa_key');
  my $rsa_rfc4716_key = File::Spec->rel2abs('t/etc/modules/mod_sftp/authorized_rsa_keys');

  my $authorized_keys = File::Spec->rel2abs("$tmpdir/.authorized_keys");
  unless (copy($rsa_rfc4716_key, $authorized_keys)) {
    die("Can't copy $rsa_rfc4716_key to $authorized_keys: $!");
  }

  my $priv_key = File::Spec->rel2abs("$tmpdir/test_rsa_key");
  unless (copy($rsa_priv_key, $priv_key)) {
    die("Can't copy $rsa_priv_key to $priv_key: $!");
  }

  unless (chmod(0400, $priv_key)) {
    die("Can't set perms on $priv_key: $!");
  }

  my $config = {
    PidFile => $pid_file,
    ScoreboardFile => $scoreboard_file,
    SystemLog => $log_file,
    TraceLog => $log_file,
    Trace => 'rsync:20 ssh2:20',

    AuthUserFile => $auth_user_file,
    AuthGroupFile => $auth_group_file,
    TimeoutIdle => $timeout_idle,

    IfModules => {
      'mod_delay.c' => {
        DelayEngine => 'off',
      },

      'mod_rsync.c' => {
        RsyncEngine => 'on',
        RsyncLog => $log_file,
      },

      'mod_sftp.c' => [
        "SFTPEngine on",
        "SFTPLog $log_file",
        "SFTPHostKey $rsa_host_key",
        "SFTPHostKey $dsa_host_key",
        "SFTPAuthorizedUserKeys file:~/.authorized_keys",
      ],
    },
  };

  my ($port, $config_user, $config_group) = config_write($config_file, $config);

  # Open pipes, for use between the parent and child processes.  Specifically,
  # the child will indicate when it's done with its test by writing a message
  # to the parent.
  my ($rfh, $wfh);
  unless (pipe($rfh, $wfh)) {
    die("Can't open pipe: $!");
  }

  my $ex;

  require File::RsyncP;

  # Fork child
  $self->handle_sigchld();
  defined(my $pid = fork()) or die("Can't fork: $!");
  if ($pid) {
    eval {
      # Give the server a chance to start up
      sleep(2);

      my $rsync_verbose = 0;
      if ($ENV{TEST_VERBOSE}) {
        $rsync_verbose = 10;
      }

      my $client = File::RsyncP->new({
        logLevel => $rsync_verbose,

        rsyncCmd => "ssh -2 -4 -a -q -x -oIdentityFile=$priv_key -oPasswordAuthentication=no -oPubkeyAuthentication=yes -oStrictHostKeyChecking=no -oCheckHostIP=no -p $port $user\@127.0.0.1 rsync",

        rsyncArgs => [
        ],
      });

      my $res = $client->remoteStart(1);
      if ($res) {
         die("Failed to connect to remote rsync");
      }

      $res = $client->go('/dev/null');
      if ($res) {
        die("Failed to receive files from remote rsync");
      } 

      $res = $client->serverClose();
      if ($res) {
        die("Failed to close connection to remote rsync");
      }
    };

    if ($@) {
      $ex = $@;
    }

    $wfh->print("done\n");
    $wfh->flush();

  } else {
    eval { server_wait($config_file, $rfh, $timeout_idle + 1) };
    if ($@) {
      warn($@);
      exit 1;
    }

    exit 0;
  }

  # Stop server
  server_stop($pid_file);

  $self->assert_child_ok($pid);

  if ($ex) {
    die($ex);
  }

#  unlink($log_file);
}

sub rsync_list_file_cvs_exclude {
  my $self = shift;
  my $tmpdir = $self->{tmpdir};

  my $config_file = "$tmpdir/rsync.conf";
  my $pid_file = File::Spec->rel2abs("$tmpdir/rsync.pid");
  my $scoreboard_file = File::Spec->rel2abs("$tmpdir/rsync.scoreboard");

  my $log_file = File::Spec->rel2abs('tests.log');

  my $auth_user_file = File::Spec->rel2abs("$tmpdir/rsync.passwd");
  my $auth_group_file = File::Spec->rel2abs("$tmpdir/rsync.group");

  my $user = 'proftpd';
  my $passwd = 'test';
  my $home_dir = File::Spec->rel2abs($tmpdir);
  my $uid = 500;
  my $gid = 500;

  # Make sure that, if we're running as root, that the home directory has
  # permissions/privs set for the account we create
  if ($< == 0) {
    unless (chmod(0755, $home_dir)) {
      die("Can't set perms on $home_dir to 0755: $!");
    }

    unless (chown($uid, $gid, $home_dir)) {
      die("Can't set owner of $home_dir to $uid/$gid: $!");
    }
  }

  auth_user_write($auth_user_file, $user, $passwd, $uid, $gid, $home_dir,
    '/bin/bash');
  auth_group_write($auth_group_file, 'ftpd', $gid, $user);

  my $timeout_idle = 15;

  my $rsa_host_key = File::Spec->rel2abs('t/etc/modules/mod_sftp/ssh_host_rsa_key');
  my $dsa_host_key = File::Spec->rel2abs('t/etc/modules/mod_sftp/ssh_host_dsa_key');

  my $rsa_priv_key = File::Spec->rel2abs('t/etc/modules/mod_sftp/test_rsa_key');
  my $rsa_rfc4716_key = File::Spec->rel2abs('t/etc/modules/mod_sftp/authorized_rsa_keys');

  my $authorized_keys = File::Spec->rel2abs("$tmpdir/.authorized_keys");
  unless (copy($rsa_rfc4716_key, $authorized_keys)) {
    die("Can't copy $rsa_rfc4716_key to $authorized_keys: $!");
  }

  my $priv_key = File::Spec->rel2abs("$tmpdir/test_rsa_key");
  unless (copy($rsa_priv_key, $priv_key)) {
    die("Can't copy $rsa_priv_key to $priv_key: $!");
  }

  unless (chmod(0400, $priv_key)) {
    die("Can't set perms on $priv_key: $!");
  }

  my $test_file = File::Spec->rel2abs("$tmpdir/test.orig");
  if (open(my $fh, "> $test_file")) {
    print $fh "Hello, World!\n";
    unless (close($fh)) {
      die("Can't write $test_file: $!");
    }

  } else {
    die("Can't open $test_file: $!");
  }

  my $config = {
    PidFile => $pid_file,
    ScoreboardFile => $scoreboard_file,
    SystemLog => $log_file,
    TraceLog => $log_file,
    Trace => 'rsync:20 ssh2:20',

    AuthUserFile => $auth_user_file,
    AuthGroupFile => $auth_group_file,
    TimeoutIdle => $timeout_idle,

    IfModules => {
      'mod_delay.c' => {
        DelayEngine => 'off',
      },

      'mod_rsync.c' => {
        RsyncEngine => 'on',
        RsyncLog => $log_file,
      },

      'mod_sftp.c' => [
        "SFTPEngine on",
        "SFTPLog $log_file",
        "SFTPHostKey $rsa_host_key",
        "SFTPHostKey $dsa_host_key",
        "SFTPAuthorizedUserKeys file:~/.authorized_keys",
      ],
    },
  };

  my ($port, $config_user, $config_group) = config_write($config_file, $config);

  # Open pipes, for use between the parent and child processes.  Specifically,
  # the child will indicate when it's done with its test by writing a message
  # to the parent.
  my ($rfh, $wfh);
  unless (pipe($rfh, $wfh)) {
    die("Can't open pipe: $!");
  }

  my $ex;

  require File::RsyncP;

  # Fork child
  $self->handle_sigchld();
  defined(my $pid = fork()) or die("Can't fork: $!");
  if ($pid) {
    eval {
      # Give the server a chance to start up
      sleep(2);

      my $rsync_verbose = 0;
      if ($ENV{TEST_VERBOSE}) {
        $rsync_verbose = 10;
      }

      my $client = File::RsyncP->new({
        logLevel => $rsync_verbose,

        rsyncCmd => "ssh -2 -4 -a -q -x -oIdentityFile=$priv_key -oPasswordAuthentication=no -oPubkeyAuthentication=yes -oStrictHostKeyChecking=no -oCheckHostIP=no -p $port $user\@127.0.0.1 rsync",

        rsyncArgs => [
          '--cvs-exclude',
        ],
      });

      my $res = $client->remoteStart(1, 'test.orig');
      if ($res) {
         die("Failed to connect to remote rsync");
      }

      $res = $client->go('/dev/null');
      if ($res) {
        die("Failed to receive files from remote rsync");
      } 

      $res = $client->serverClose();
      if ($res) {
        die("Failed to close connection to remote rsync");
      }

      my $stats = $client->statsFinal();
use Data::Dumper;
print STDERR "stats: ", Dumper($stats), "\n";

    };

    if ($@) {
      $ex = $@;
    }

    $wfh->print("done\n");
    $wfh->flush();

  } else {
    eval { server_wait($config_file, $rfh, $timeout_idle + 1) };
    if ($@) {
      warn($@);
      exit 1;
    }

    exit 0;
  }

  # Stop server
  server_stop($pid_file);

  $self->assert_child_ok($pid);

  if ($ex) {
    die($ex);
  }

  unlink($log_file);
}

1;
