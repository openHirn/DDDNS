------------------------------------------------------------------------------
  DDDNS library Readme
  Repo: internal://git.dev:git/lib/php/DDDNS

  David Herminghaus, www.david-herminghaus.de
------------------------------------------------------------------------------

Contents:
=========
1. ABOUT
2. PREREQUISITES
3. TECHNICAL
4. SETUP
5. ROADMAP

1. ABOUT
========
DDDNS is WA replacement for standard remote NSUPDATE procedures (involving
RNDC etc.). It results from requirements in a limited environment.

2. PREREQUISITES
================
Standard LAMP environment.

3. TECHNICAL WORKFLOW
=====================

1. READ
Arbitrary clients may request the script via http (where it is your
obligation to provide a secured channel, either way!) and, in return, see
information on the last successful dynamic IP update.

2. UPDATE
If adding a "GET" (todo: "POST"/"PUT") request parameter, keyed "update",
with an arbitrary non-empty value, the request is considered an update
and a local NSUPDATE is attempted. On success, the remote IP is stored to
the database as the new dynamic IP.

4. SETUP
========
1. Set up a database using the included sql script (first, replace user names in
   the view definitions!)
2. Configure a secure web server, add whatever authentication, place index.php
   where it belongs, configure your internals, start the web server.
3. Optionally configure dddnsAlive.php as of your situation, ensure mail
   outgoing mail works and add a cron task frequently running the
   script like so:
     
     */5 * * * * [/path/to/]php /path/to/ddnsAlive.php

5. ROADMAP
==========
Maintenance fixes, no general drop since the approach works fine and
off-standards (which is no implication at the moment). Also, a web interface
is nice to have compared to SSH solutions and can even be called from a
non-*x machine.

@todo:
* Split off configuration
* Separate templates from actual config files (common standards)
