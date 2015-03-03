DDDNS installer readme

1. Intent, preface
The installer files in this directory are designed to set up database tables
as of your log configuration. Thus please first follow any other setup step
prior to taking this step.
Please run the installer always
* after you have finished your individual DDDNS configuration,
* after you have updated DDDNS to a newer version.

2. Prerequisites
The installer will only work if you have set up a logging database and provided
the corresponding DB credentials in your dddns.config.inc file.

3. How to launch
Assuming your working directory is the one containing this file,
you would enter:

/path/to/php install.php

This is pretty much all.
