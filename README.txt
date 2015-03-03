------------------------------------------------------------------------------
  DDDNS library Readme
  git://internal/git/lib/php/DDDNS

  Copyright (c)
  - David Herminghaus, www.david-herminghaus.de
  - Torsten Demmig, www.larus-software.de
  Published and publically licensed under CC-NC-BY-SA,
    see http://creativecommons.org/licenses/by-nc-sa/4.0/
  All other rights reserved.
  
------------------------------------------------------------------------------

Contents:
=========
1. INTRODUCTION, IMPORTANT NOTE
2. PREREQUISITES, LIMITATIONS
3. SETUP
4. BASIC CLIENT USAGE, CLIENT-SIDE REST API
5. PARAMETERS AND OVERRIDES
6. TYPICAL WORKFLOW EXAMPLE
7. API


1. INTRODUCTION, IMPORTANT NOTE
===============================
DDNS is a script collection that takes care of and only of dynamic DNS updates.
A simple HTTP REST API allows you to build your own dynamic DNS service and
use it with any device that can issue and handle a (secure) HTTP request.

This script DOES NOT PROVIDE ANY AUTHENTICATION OR SECURITY mechanisms. It is
at your own obligation, and really recommended for any use case exposed to the
Internet, that you secure the HTTP transport layer (i. e. use TLS or VPN!)
as well as protect the web frontend by HTTP or any other working
authentication.

The evil is using the Internet, and you have been told now.


2. PREREQUISITES, LIMITATIONS
=============================

* Apache 2 (or maybe compatible PHP enabled web servers; none tested yet).
  With mod_rewrite (or equivalent solutions; see ./web/.htaccess)
  you will achieve even more HTTP REST compliance.
* PHP 5. The current version does not yet use PDO, which may become
  an issue in PHP versions > 5.5. Also, system calls (shell_exec)
  must be allowed by your php configuration.
* A working DNS, either
  * BIND 9
  * PowerDNS 3 with either
    * a MySQL database backend (set $config['ddns']['type'] to 'pdns'
      and add the required credentials. See ./includes/class.dddns.dns.pdns.inc)
    * a BIND backend, leave 'type' to generic and make sure both nsupdate
      and dig work (see also below)
* The dnsutils package components "nsupdate" (for BIND and PDNS/BIND)
  and, in either case, "dig". Make sure to add the correct paths for both to
  your config unless they are both located in "/usr/bin" as by default.
* A public web server with a static IP hosting the said applications.
* You should be familiar with BIND's basic concepts.

* A MySQL server is OPTIONAL, yet recommended also for logging, health checks
  etc. DDDNS has been successfully tested with MySQL 5.5.

Also note that DDDNS, although trying to be as generic as possible in details,
effectively only handles IPv4 by now. This may change.



3. SETUP
========

3.1. DNS

DNS is a complex topic not to be covered in detail here. There are basically
two DNS-side scenarios I am aware of:

A) Own DNS on premises, official SOA entries at your national IP registry.
   (No ISP or hoster "in the middle".)

   1. Update your named.conf.local as of ./examples/named.conf.local
   2. Unless your DNS runs on the same machine as the web frontend,
      you should consider RNDC-secured updates (see BIND manual).
   3. Add or modify the appropriate zone file(s) as suggested in
      ./examples/dns.standalone.hosts.
   4. Test and apply changes.

B) Domain DNS basically served by some other host, the records of which can be
   modified. (Likely if you have a hosting package at any ISP.)
   
   1. Set up your dynamic BIND server (not covered here).
   2. Add or update the zone file(s) corresponding with your dynamic
      domains as suggested in ./examples/dns.standalone.hosts.
   3. Test and commit the changes.
   4. Update the main DNS' zone record for the domain(s) in question
      as suggested in ./examples/dns.isp.hosts. I. e., add the
      NS entries so the main DNS server can delegate requests for your
      dynamic domains to your dynamic DNS host.
   5. Apply the changes to your "main" DNS.


3.2. WEB SERVER

Set up a new (virtual) web server for the DDDNS frontend. It will provide
basic IP lookup and an update interface for your client device(s).

1. Create a ./web/.htaccess file based on ./examples/.htaccess.
   If you want to stick to HTTP basic auth, also create a suitable .htpasswd
   file. Otherwise modify .htaccess to use your preferred auth backend.
   (All not covered here in detail, but you will manage to look it up.)
2. Create a ./config/dddns.config.inc file from the template in ./examples.
   Make sure to double-check every value matches your existing facilities.
   All config options are documented in the example file.
   Note that an RNDC key path is optional and primarily makes sense if
   the DNS is running on a different machine (not on localhost).
3. Make sure your web server config can handle index.php as default directory
   index and has read access to all documents in ./web, ./includes and ./config.
   If you want to enable path based host detection, uncomment the rewrite section
   in .htaccess and enable mod_rewrite (aka "clean urls").
4. Set the DOCUMENT_ROOT to ./web.
5. (Re)start the web server.


3.3. DATABASE

The included pdo log backend works with any ANSI SQL compliant database,
as long as a PHP PDO driver exists for it. The extension will also auto-create
the required schema. To make this work, you need to:

1. Set up a database and a user with all privileges on it.
2. If necessary, install the PDO driver for your database type.
3. Read and mark the notes in the ./_install/README.txt and
   when you are ready, install the DB tables.

For other database driven extensions, additional steps may be required.
Please check with the corresponding extension class documentation.


3.4. HEALTH CHECKS

In the ./tools folder you will find two scripts which allow for CLI (and thus
cron enabled) healthchecks both using bash/curl or PHP (e. g. if your server
lacks curl for some reason).

Both script's aren't but wrappers to a standard DDDNS API call, so you can
use them both directly or as a template for custom improvements. Also,
both scripts take the same arguments on the command line, these
are also explaned inline.

Note that health checks will not work unless you have a working log
database set up.


4. BASIC CLIENT USAGE, CLIENT-SIDE REST API
===========================================

DDDNS provides a simple REST CRUD API. Thus it can be used with any contemporary
HTTP-enabled client. In respect for older HTTP variants and due to the fact
that there is no veritable "create" action making sense via HTTP,
both "PUT" and "POST" represent the unique "update" action.

Also note the additional "action" override and further parameters below.

4.1. READ (action=show, default action for GET)

To find out about the dynamic IP currently assigned to a certain host, you can

* Perform an ns lookup against your DNS server (duh!).
* Dig into your SQL database (see the "last update" view).
* Perform a GET request against the REST API:

A. With host detection (see SETUP):

	REQUEST METHOD:
		GET
	URL:
		http(s)://[username:password@]your.dddns.host/host.in.question

	Example curl representation:
		curl https://user:password@your.dddns.host/host.in.question

B. Without host detection (see SETUP):

	URL:
		http(s)://[username:password@]your.dddns.host?host=host.in.question

	Example curl representation:
		curl https://user:password@your.dddns.host?host=host.in.question


4.2. CREATE/ADD (action=add, default POST action in forced mode)

To add an IP record (do not mismatch with UPDATE below!) you will issue a POST
request. Mind the notes above and below on "forced HTTP compatibility".
To override IP autodetection from the client's request, you can set
one or more explicite updateIPs. See "5. Parameters and overrides".

A. With host detection (see SETUP):

	REQUEST METHOD:
		POST
	URL:
		http(s)://[username:password@]your.dddns.host/host.in.question
	Data:
		updateIP=[one ore more IPs to add; optional]

	Example curl representations:
		- curl -X POST https://user:password@your.dddns.host/host.in.question
		  "Add a record for my client's remote IP to host.in.question"
		- curl -X POST -d 'updateIP=12.34.56.78' https://user:password@your.dddns.host/host.in.question
		  "Add a record for 12.34.56.78 to host.in.question"

B. Without host detection

	URL:
		http(s)://[username:password@]your.dddns.host
	Data:
		host=host.in.question
		updateIP=[one ore more IPs to add]

	Example curl representation:
		- curl -X POST -d 'host=host.in.question' https://user:password@your.dddns.host
		  "Add a record for my client's remote IP to host.in.question"
		- curl -X POST -d 'host=host.in.question&updateIP=12.34.56.78' https://user:password@your.dddns.host
		  "Add a record for 12.34.56.78 to host.in.question"


4.3. UPDATE (action=update, default PUT and POST action except in forced mode)

Updates are triggered by issueing a POST or PUT request (in forced mode:
by a PUT request).

A. With host detection (see SETUP):

	REQUEST METHOD:
		PUT (and POST in non-forced mode)
	URL:
		http(s)://[username:password@]your.dddns.host/host.in.question
	Data:
		updateIP=[optional; one or more explicite IPs to replace existing ones]

	Example curl representations:
		- curl -X PUT https://user:password@your.dddns.host/host.in.question
		  "Remove all current records except my current IP and eventually add the latter"
		- curl -X PUT https://user:password@your.dddns.host/host.in.question?updateIP=12.345.6.7
		  "Delete all current IP records except 12.345.6.7 and add the latter, if required"
		- curl -X PUT https://user:password@your.dddns.host/host.in.question?updateIP[]=98.76.54.3&updateIP[]=12.345.6.7
		  "Delete all current IP records except these two and add them, if required"

B. Without host detection

	URL:
		http(s)://[username:password@]your.dddns.host
	Data:
		host=host.in.question
		updateIP=[optional; one or more explicite IPs to replace existing ones]

	Example curl representation:
		- curl -X PUT https://user:password@your.dddns.host?host=host.in.question
		  "Remove all current records except my current IP and eventually add the latter"
		- curl -X PUT https://user:password@your.dddns.host?host=host.in.question&updateIP=12.345.6.7
		  "Delete all current IP records except 12.345.6.7 and add the latter, if required"
		- curl -X PUT https://user:password@your.dddns.host?host=host.in.question&updateIP[]=98.76.54.3&updateIP[]=12.345.6.7
		  "Delete all current IP records except these two and add them, if required"


4.4. DELETE

Consequently you will use an HTTP DELETE statement here.

You may limit the IP records to one or more IPs by providing an explicite
"deleteIP" parameter (single or array). If you don't, all records for
this host will be erased.

A. With host detection (see SETUP):

	URL:
		http(s)://[username:password@]your.dddns.host/host.in.question
	Data:
		deleteIP=[optional IP/list to be deleted]

	Example curl representations:
		- curl -X DELETE https://user:password@your.dddns.host/host.in.question
		  "Delete any record for host.in.question"
		- curl -X DELETE https://user:password@your.dddns.host/host.in.question?deleteIP=1.2.3.4
		  "Delete 1.2.3.4 record from host.in.question, if it exists"
		- curl -X DELETE https://user:password@your.dddns.host/host.in.question?deleteIP[]=1.2.3.4&deleteIP[]=2.3.4.5
		  "Delete each 1.2.3.4 and 2.3.4.5 records from host.in.question,
		  in case they exist"

B. Without host detection:

	URL:
		http(s)://[username:password@]your.dddns.host
	Data:
		host=host.in.question
		[deleteIP=optional IP/list to be deleted]

	Example curl representations:
		- curl -X DELETE https://user:password@your.dddns.host?host=host.in.question
		  "Delete any record for host.in.question"
		- curl -X DELETE https://user:password@your.dddns.host?host=host.in.question&deleteIP=1.2.3.4
		  "Delete 1.2.3.4 record from host.in.question, if it exists"
		- curl -X DELETE https://user:password@your.dddns.host/host.in.question?host=host.in.question&deleteIP[]=1.2.3.4&deleteIP[]=2.3.4.5
		  "Delete each 1.2.3.4 and 2.3.4.5 records from host.in.question,
		  in case they exist"

5. PARAMETERS AND OVERRIDES

The following parameters can be passed in the HTTP data set (or query
string) as key/value pairs.

action*
	Since many clients (especially routers, but also e.g. wget) have limited
	HTTP capabilities, the actually desired method can be set with this
	parameter. It will override the REQUEST_METHOD detection explained above.
	
	Allowed values and their request method representations:
	
	add    => POST (in forced mode)
	update => POST/PUT (in default mode; in forced mode only PUT)
	delete => DELETE
	show   => GET
	
	Example URL to read an entry via POST/PUT request:
		http(s)://[username:password@]your.dddns.host
	Data:
		host=host.in.question&action=show

clientExtra
clientKey
clientNote
	Fields reserved for DB logging and other customizations.
	May contain arbitrary values up to 512 bytes.

host
	This parameter must always provide the host to be updated.
	Can be omitted if the path based host name detection is enabled
	and a valid host is given in the URL.

output
	Override the internal standard format. The value must represent an existing
	output class ("html" by default) to make an override work.
	Shipped output formats:
	* html (default)
	* json

updateIP*
	To override the REMOTE_IP detection (the default update mechanism),
	set "ip" to any valid IP pattern. You can also provide multiple IPs.
	Note that, if you want to have your client's request IP also being regarded,
	you need to explicitely supply it again, because "overriding" is not
	"merging".

	Example URL to set a forced IP using a GET request:
		https://user:password@your.ddns.host?action=update&host=dyn.example.com&updateIP=123.45.6.7

	Example URL to set multiple forced IPs using a GET request:
		https://user:password@your.ddns.host?action=update&host=dyn.example.com&updateIP[]=123.45.6.7&updateIP[]=2.3.4.5

show
	Since there are more informational actions than "show" (which is also the
	default	action for all "GET" requests), this additional parameter extends
	the informational commands.
	Available show actions are:
	
	check
		Tells whether (or not) a client host has successfully updated
		(or added) its IP. Requires a database backend for logging,
		see "SETUP".
		
		Note that this action requires additional parameters to work
		(see "lookback", "mailto" and "cleanup").
		
		Example to check for updates on "dyn.example.com" within the last two
		minutes and send a mail to root@example.com, if there weren't.
			https://user:password@your.ddns.host?action=show&show=check&host=dyn.example.com&lookback=120&mailto=root@example.com
		
		Example like before, but check for two hours and disable all host
		records in case of a failure:
			https://user:password@your.ddns.host?action=show&show=check&host=dyn.example.com&lookback=7200&mailto=root@example.com&cleanup=true

		Example like before, sending out a warning mail even if this has already
		happened.
	
cleanup
	Additional parameter for "action=show&show=check".
	
	If a check finds out that updates were missing within the "lookback" time
	period, and this parameter is "true", all remaining records will be
	removed from DNS. This improves security (by reducing the risk of unexpected
	endpoints), however, it should be considered carefully, e.g. if you
	work with multiple end points (and not with simple 1:1 host:ip logic).
	Note: "true" must be an all lowercase string.

lookback
	Additional parameter for "action=show&show=check".
	
	Pass an integer to define the time frame ("last n seconds") in which
	the most recent update must have happened to not trigger a warning.

mailto
	In order not to have mail recipients in the configuration and thus to
	allow more flexible calls, you may and must provide at least one
	recipient here. (Or multiple recipients separated by semicolon.)
	This is still limited, however, it will work.
	Unless you omit this parameter.

*These parameters override the default behaviour and require you to set the
'ignore_overrides' flag in your config file to FALSE for security reasons.


6. TYPICAL WORKFLOW EXAMPLE
===========================

Usually you will install DDDNS as suggested above. Then, you will set up a
cronjob running inside your local LAN (the one to be reached with a dynamic
IP) which frequently sends a PUT request to the web frontend.


6.1. LINUX
----------

A. cURL (recommended)

Using any Linux client is usually the simplest idea. All you need is a crontab
entry like this:

*/5 * * * * curl -X PUT https://user:password@your.dddns.host/dynamic.host.name

This will set an A record corresponding with your LAN's public WAN IP for
the host "dynamic.host.name" (assuming you have enabled HTTP auth and supplied
valid credentials, which you both should). For TLS, which is also strongly
recommended, you might have to provide your CA's root certificate and/or
the web server's certificate in your client's certificate store (in Debian e.g.
at /etc/ssl/certs).

B. wget

If you want (or need) to use wget instead, you are likely to set the
allow_overrides config parameter to true, since wget is not capable of PUT
requests. In most setups you will have to provide the ca and/or your server'
certificate.

*/5 * * * * wget --ca-certificate=/path/to/ca-certificate --certificate=/path/to/server_cert --spider https://user:password@your.dddns.host/dynamic.host.name?action=update


6.2. WINDOWS
------------

If your only 24/7 machine is a Windows server (SBS etc.), you can

- Use one of the existing "cURL for Windows" solutions.
- Use Gnu32WIN tools, which provide a wget. See wget notes above.
- Probably use Windows Powershell (and read the manuals, I didn't bother).

Also note the general suggestions from the Linux section.


6.3. OTHER
----------

Many routers include DDNS functionality today. However, few have a really
serious implementation (sorry to say this). Still today, even expensive
mainstream routers still do not care for TLS anyhow, and still there is
no truly standardized approach for a "DDNS over HTTP(S)" protocol.

This is why not even the popular German "Fritzbox" is treated here. We simply
did not get it working with the (actually few and simple) requirements above.

And it is why we think that at this here point, figuring a way to do this
with any embedded system is up to you and only you, let alone thinking of
the numerous custom firmware replacements in the wild.

In the end, most embedded system are Linux systems and thus you will find a way
if you regard the above suggestions. From what I tried, you can manage
any embedded system (even busybox) to somehow work with DDDNS, as long as
you do have root shell access. And if you don't, a Rasperry Pi makes a good
bargain anyway.


7. API
======

At the moment there are three public methods.

1. Constructor
   
   Creating a new instance is done like this:
   
   a) To retrieve all arguments from the HTTP request and
      use your standard configuration in ./config/dddns.config:
      
      $dddns = new DDDNS();
   
   b) Create an instance with explicite arguments and the
      standard configuration:
      
      $dddns = new DDDNS(array('host' => 'dyn.example.com'));
      
   c) Create an instance with standard request arguments
      but a different config file:
      
      $dddns = new DDDNS(NULL, '/path/to/different/config.file');
      
   
   On instantiation, all required parameters and settings will already
   be prepared and configured (like "action to perform", host, new IP).

2. Standard execution (detect from request and/or arguments)
   
   To finally run the designated and prepared action(s), you will usually
   call the "execute" method:
   
     $dddns->execute();
       Will perform any prepared/configured action and update
       internal result data.

     $result = $dddns->getResult();
       $result will be a DDDNSresult object afterwards.
   
   To get parsed output (html by default, optionally JSON or any
   custom output format provided by an output extension):
   
     $output = $dddns->getOutput();
       $output will be either html or whatever format was requested
       from a client by the "output" argument.
     
     $output = $dddns->getOutput('html');
       $output will be explicitly html, accomplished by
       DDDNSoutputHtml (class.dddns.output.html.inc).
      
     $output = $dddns->getOutput('json');
       $output will explicitely be json, accomplished by
       DDDNSoutputJson (class.dddns.output.json.inc).
     
     $output = $dddns->getOutput('custom);
       $output will be whatever DDDNSoutputCustom
       (class.dddns.output.custom.inc) returns.
   
   To have the results parsed and sent right away:
   
     $output = $dddns->sendOutput();
       Will automatically have sent the content of $output, preceeded by
       the output extensions related HTTP headers, to stdout.
       
     $output = $dddns->sendOutput('json');
       Will automatically have sent the content of $output, in forced
       JSON format (etc., see getOutput()).

3. Partial execution
 
   You can also perform single dns actions:
   
   1. DELETION
   
      a) Delete the IP(s) provided in the argument parameter "deleteIP":
      
         $result = $ddns->delete();
         
         (Returns TRUE or FALSE, depending on successful deletion).
      
      b) Delete one or more explicitely given IPs from $host's record:
      
         $result = $dddns->delete('1.2.3.4');
         $result = $dddns->delete(array('1.2.3.4', '6.7.8.9'));
         
         (Returns TRUE or FALSE, depending on complete successful deletion).
   
   2. ADDITION
   
      a) Create an additional record for the IP detected from the request
         (or provided in the optional "ip" override argument):
         
         $result = $dddns->add();
         
      b) Create an additional record for $host from one or more
         explicitely provided IPs:
         
         $result = $dddns->add('2.3.4.5');
         $result = $dddns->add(array('2.3.4.5', '3.4.5.6'));
      
      (All returning TRUE or FALSE, depending on overall success.)

   3. UPDATING
   
      Updates are basically a combination of "add" and "delete".
      The given IP(s) (determined from request/parameters or a list
      of new(!) IPs you pass to this method, all IPs not matching
      will be deleted and the ones missing will be added subsequently.
      
      Note the different result structure, it provides additional information.
   
      a) Trigger an update to your current client IP (read from the request
         or provided in the optional "ip" override argument):
         
         $result = $dddns->update();
         
      b) Update records for $host from one or more explicitely provided IPs:
         
         $result = $dddns->update('2.3.4.5');
         $result = $dddns->update(array('2.3.4.5', '3.4.5.6'));
      
      Both a and b will return a result telling whether an update was necessary,
      and if so, whether it succeeded.
    
    4. RETRIEVING INFORMATION
    
      The following methods provide information about the current DDDNS object:
      
	  check()
	    Whether or not the current host has submitted any new IP within a
	    certain time frame.

	  lookup()
	    Returns an array of all DNS records currently found for the
	    given host. I. e., an "nslookup" result.

      getHeaders()
      	Returns a keyed array of current headers and their values related to
      	the request. Currently limited to the HTTP result status code.
     
	  The following methods are (hopefully) implemented by any output extension.
	  They can be addressed as
	  
	  	$dddnsObject->output[$format]->$method()
	  
	  where $format must be an implemented output ("html" or "json")
	  extension and $method is one of the following:

      getHeaders()
      	Returns a keyed array of current headers and their values related to
      	the output. May, but not necessarily must also contain
      	request related data.
      
      output()
        Returns a DDDNSresult object, parsed to the actual output format,
        and optionally sends it to stdout.
      	
      sendHeaders()
        Sends all headers as stored in the actual output object
        to a HTTP client.
      
      setHeaders()
        Adds/updates headers of the actual output object.

      See also inline documentation for all described methods.

4. Templating

If you choose the "html" output format, the html output extension will parse
the template configured in $config['output']['html']['formatter'].
'default' resolves to ./templates/html.default.tpl.php, while 'custom'
would use a template ./templates/html.custom.tpl.php.
	