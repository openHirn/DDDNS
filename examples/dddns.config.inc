<?php

/**
 * @file
 * Example configuration for DDDNS library.
 *
 * To override or enable an item, uncomment and customize it.
 * The commented items show system defaults or suggestions.
 */

$config = array(

  /* GENERAL */

  // Whether to allow URL overrides. Set to TRUE if your client
  // is not capable of properly using HTTP request methods and/or you
  // want/need to use HTTP GET with query parameters.
  // See README for parameters and notes.
  // 'allow_overrides' => FALSE,

  // Whether to force correct evaluation of HTTP REQUEST_METHOD.
  // By default, both POST and PUT are mapped to the "update" action.
  // This is because PUT is still said to have sketchy support.
  // If you prefer POST request to simply add an IP, you can set this parameter
  // to TRUE. POST will then trigger a bare "add" (Which is closer to HTTP
  // protocol standards). PUT remains unchanged in any case.
  // 'force_http_post' => FALSE,

  // The correct and HTTP compliant result behaviour would be to return
  // a HTTP 304 ("not modified") if a requested update was not necessary.
  // However this causes unpredictable behaviour in almost any HTTP
  // client, as it probably involves locally cached results which
  // not always reflect the actual changes in DNS.
  // This is why, by default, even an unnecessary update will receive
  // a HTTP 200 status by default.
  // If you have reasons to disable this workaround, you may do it here
  // by setting the following to TRUE:
  // 'allow_http_not_modified' => FALSE,

  /* EXTENSIONS */

  // Please also refer to additional config documentation of any extension
  // plugin, if provided.

  /* DDNS */

  // DDNS is a mandatory extension. It defaults to the "generic" class
  // (see ./includes/class.dddns.dns.generic.inc).
  // It comes with predefined values, which are reflected below.

  // Uncomment and customize if your premises differ from the defaults.

  /*
  'ddns' => array(

    // The nameserver backend type. The related classfile must
    // exist in ../indludes/ as dddns.dns.type.inc where the class name
    // is DDDNSdnsType (see ../includes/dddns.dns.generic.inc).
    // Additional per-DNS-type config must be inside this array.
    //
    // Also look for information on additional config options
    // in the actual class file.
    //
    // 'type' => 'generic',

    // The nameserver to perform the NSUPDATE against (i. e., the DDNS).
    //
    // 'nameserver' => 'localhost',

    // DNS TTL in seconds. After this period, a DNS client is supposed to refresh
    // their data. Thus find yourself a balance between server destruction
    // and a time frame you can live with from remote.
    //
    // 'ttl' => 60,

  ),
  */

  /* OUTPUT */

  // Override the default HTML output template or add output types.
  // 'output' => array(
  //   # The following uses the template file
  //   #  "../templates/DDDNS.output.html.default.inc"
  //   'html' => array(
  //     'formatter' => 'default',
  //   ),
  //   # The following uses the output class (just make it so!).
  //   #  "./DDDNS.output.json.inc"
  //   'json' => array(
  //     'formatter' => 'default',
  //   ),
  // ),

  /* LOG */

  // DDDNS will also work without a database backend. In this case you cannot
  // review any update details (timestamp, user etc.) and also health checks
  // will not be possible.
  // To enable the DB backend, uncomment (and customize) the following section.
  /*
  'log' => array(

    // You may enable more than one log backend. Note that the first entry
    // will be the default source for "check" and other monitoring requests.

    'pdo' => array(
      'dsn' => '',
      'user' => '',
      'pass' => '',

      // Optionally limit logging to certain actions.
      // 'limitLogEvents' => array('show', 'update', 'add', 'delete', 'check'),

    ),

    // Optional: Second log backend. Core class will write to all provided
    // log backends.
    // 'custom' => array(...),

  ),
  */

);
