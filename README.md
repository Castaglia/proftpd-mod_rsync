proftpd-mod_rsync
=================

Status
------
[![Build Status](https://travis-ci.org/Castaglia/proftpd-mod_rsync.svg?branch=master)](https://travis-ci.org/Castaglia/proftpd-mod_rsync)
[![Coverage Status](https://coveralls.io/repos/github/Castaglia/proftpd-mod_rsync/badge.svg?branch=master)](https://coveralls.io/github/Castaglia/proftpd-mod_rsync?branch=master)
[![License](https://img.shields.io/badge/license-GPL-brightgreen.svg)](https://img.shields.io/badge/license-GPL-brightgreen.svg)


Synopsis
--------

The `mod_rsync module` for ProFTPD is intended to support the rsync protocol,
tunneled over an SSH connection; the `mod_rsync` module thus uses the
[`mod_sftp`](http://www.proftpd.org/docs/contrib/mod_sftp.html) module for
ProFTPD.

For further module documentation, see [mod_rsync.html](https://htmlpreview.github.io/?https://github.com/Castaglia/proftpd-mod_rsync/blob/master/mod_rsync.html).

**NOTE**: The `mod_rsync` module is **not completed**, and is in a state of
development.  Please do *not* attempt to use it for any production service.

