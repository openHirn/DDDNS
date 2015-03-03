<?php

/**
 * @file
 * Dynamic DNS library for BIND/NSUPDATE enabled LAMP hosts.
 *
 * Global @todo:
 * - Refactor -> PDO
 */

require_once dirname(__FILE__) . '/class.dddns.exception.inc';
require_once dirname(__FILE__) . '/class.dddns.result.inc';

class DDDNS {

  const HTTP_STATUS_OK = 200;
  const HTTP_STATUS_CREATED = 201;
  const HTTP_STATUS_NOT_MODIFIED = 304;
  const HTTP_STATUS_BAD_REQUEST = 400;
  const HTTP_STATUS_NOT_FOUND = 404;
  const HTTP_STATUS_GONE = 410;
  const HTTP_STATUS_INTERNAL_SERVER_ERROR = 500;

  private $config = array(
    // Default to the shipped extensions (except DB).
    'ddns' => array(
      'type' => 'generic',
      'nameserver' => 'localhost',
      'ttl' => 60, // 1 minute.
      'update_path' => '/usr/bin/nsupdate',
      'dig_path' => '/usr/bin/dig',
    ),
    'output' => array(
      'html' => array(
        'formatter' => 'default',
      ),
      'json' => array(
        'formatter' => 'default',
      ),
    ),
  );

  // Request data.
  private $request = array();

  // Arbitrary k=>v storage.
  private $data = array();

  // Track HTTP result code.
  private $httpResultStatus;

  // DB abstraction interface (NOT DNS related data, see $dns).
  private $db;

  // DNS abstraction interface.
  private $dns;

  // Result data.
  private $result;

  // Output abstraction.
  public $output = array();

  // Log backend abstraction.
  private $log = array();

  /**
   * Constructor.
   *
   * @param array $args
   *   Parameters and overrides, see documentation. If ommitted,
   *   $_REQUEST is evaluated.
   * @param string $configPath
   *   Optional path to a config file. If ommitted, a default path is set.
   *   @see loadConfig()
   */
  function __construct($args = NULL, $configPath = NULL) {
    if (!isset($args)) {
      $args = $_REQUEST;
    }

    // Prepare result data.
    $this->result = new DDDNSresult();

    if ($this->loadConfig($args, $configPath)) {
      // Make sure a host has been provided.
      if (empty($this->request['host']) || is_array($this->request['host'])) {
        throw new DDDNSexception(
          'You must provide exactly one "host" parameter which must not be empty.',
          DDDNS::HTTP_STATUS_BAD_REQUEST
        );
      }

      // Unify host parameter and import this and more into the result object.
      $this->result->set('host', ($this->request['host'] = strtolower($this->request['host'])))
        ->set('nameserver', $this->config['ddns']['nameserver'])
        ->set('user', @$this->request['user']) // Accepted to possibly be NULL.
        ->set('remoteIP', $this->request['remoteIP'])
      ;

      $this->dns = $this->loadExtension('dns', $this->config['ddns']['type'], $this->config['ddns']);

      // Always check initial DNS status.
      $this->result->setDNS('previous', $this->getDnsStatus('initial'));

      // Add configured output and log extension(s).
      foreach (array('output', 'log') as $section) {
        $sectionConfig = @$this->config[$section];
        if (!empty($sectionConfig)) {
          foreach (array_keys($sectionConfig) as $type) {
            $this->{$section}[$type] = $this->loadExtension($section, $type, $sectionConfig);
          }
        }
      }
      return $this;
    }
    else {
      throw new DDDNSexception('No valid config could be loaded.');
    }

  }

  /**
   * Loads and parses configuration from a file and processes request data.
   *
   * @param array $args
   *   Parameters and overrides, see documentation.
   * @param string $configPath
   *   Optional path to a config file.
   *   Defaults to '[class file location]/../config/dddns.config.inc'.
   */
  private function loadConfig($args, $configPath = NULL) {

    if (empty($configPath)) {
      $configPath = dirname(__FILE__) . '/../config/dddns.config.inc';
    }

    if (is_readable($configPath)) {
      include $configPath;
    }

    // A valid config file would provide $config as an array.
    // This is our basic validation.
    if (isset($config) && is_array($config)) {

      $this->config = array_merge($this->config, $config);

      // Evaluate request metadata.
      $this->processRequest();

      // @todo move to function "parse params" etc. or to processRequest (structural)

      $params = array(
        'action' => FALSE,
        'updateIP' => FALSE,
        'host' => TRUE,
        'clientNote' => TRUE,
        'clientKey' => TRUE,
        'clientExtra' => TRUE,
        'deleteIP' => TRUE,
        'output' => TRUE,
        'show' => TRUE,
        'lookback' => TRUE,
        'mailto' => TRUE,
        'cleanup' => TRUE,
      );

      // Parse parameters from $args explicitely.
      foreach($params as $k => $v) {
        if (
          isset($args[$k])
          &&
          (
            // Check whether this parameter is a normal parameter
            $v

            ||

            // or requires the override flag to be configured.
            // @see ../examples/dddns.config.inc
            TRUE === @$this->config['allow_overrides']
          )
        ) {

          // Sanitation and result storage:
          switch ($k) {

            case 'updateIP':
            case 'deleteIP':
              // Strip anything that is not part of an IPv4 or v6 from any IP value.
              $sanitized = preg_replace(
                '#(^[^a-z0-9:]|[^a-z0-9\.:]|[^a-z0-9:]$)#i',
                '',
                $args[$k]
              );
              break;
            case 'host':
              // Sanitize client-provided host name as of RFC 1123 (and 952).
              $sanitized = preg_replace('#(^[^a-z0-9]|[^a-z0-9\.\-]|[\.\-]$)#i', '', $args[$k]);
              break;
            case 'output':
              // Alphanumeric camelcase only due to class name conventions.
              $sanitized = preg_replace('#(?:^[^a-z]|[^a-zA-Z0-9])#', '', $args[$k]);
              break;
            case 'show':
              $sanitized = in_array($args[$k], array('check')) ? $args[$k] : '';
              break;
            case 'lookback':
              $sanitized = intval($args[$k]);
              break;
            case 'mailto':
              $sanitized = preg_replace('#[^a-zA-Z0-9@\.\-_]#', '', $args[$k]);
              break;
            case 'cleanup':
              // Exception for bool type: Create an explicit boolean from string.
              $args[$k] = $sanitized = $args[$k] === 'true';
              break;
            default:
              // Minimum sanitation is to strip whitespace etc. in order
              // to prevent sprintf exploits for shell_exec attacks etc.
              // This should not be relied upon alone.
              $sanitized = preg_replace('#\s#', '', trim($args[$k]));
          }

          // Only set params if the sanitized value matches the original value.
          // This is an attempt to prevent unwanted results by mistakes.
          // @todo @debug Re-consider.
          if ($sanitized == $args[$k]) {
            $this->request[$k] = $sanitized;
          }
        }
      }

      // Exception: "show" triggers additional GET actions.
      // (Due to limited request methods; mapping to GET makes most sense.)
      if ($this->request['action'] == 'show' && !empty($this->request['show'])) {
        $this->request['action'] = $this->request['show'];
        unset($this->request['show']);
      }

      // Pass final params to result object.
      foreach (array_keys($params) as $k) {
        if (isset($this->request[$k])) {
          if (in_array($k, array('updateIP', 'deleteIP'))) {
            $this->result->setIP($k, $this->request[$k]);
          }
          else {
            $this->result->set($k, $this->request[$k]);
          }
        }
      }

      return TRUE;
    }
    return FALSE;
  } // function loadConfig()

  /**
   * Sets instance properties from $_SERVER, if possible.
   *
   * Tries to determine the remote (client) IP and the action to perform
   * from HTTP request metadata.
   */
  private function processRequest() {

    // Set a safe default action.
    $this->request['action'] = 'show';
    // Set some more harmless defaults.
    // @todo @IPv6
    $this->request['remoteIP'] = '127.0.0.1';
    // Remember our call date.
    $this->request['requestTime'] = time();

    // Map HTTP request methods to actual internal actions.
    // See also override parameter "action".
    if (isset($_SERVER['REQUEST_METHOD'])) {
      switch($_SERVER['REQUEST_METHOD']) {
        case 'PUT':
          $this->request['action'] = 'update';
          break;
        case 'POST':
          $this->request['action'] = TRUE === @$this->config['force_http_post'] ? 'add' : 'update';
          break;
        case 'DELETE':
          $this->request['action'] = 'delete';
          break;
        // Default has already been set above.
      }
    }

    // Determine host from path, if possible.
    // The first "part" of the path is always considered the hostname
    // to work on. Requires a rewrite rule, @see .htaccess.
    if (
      isset($_SERVER['REQUEST_URI'])
      &&
      preg_match('#^/(?<host>.*?)(?:\?|/|$)#', $_SERVER['REQUEST_URI'], $m)
    ) {
      $this->request['host'] = $m['host'];
    }


    // Determine IP from request metadata.
    // See also override parameter "ip"; remote IP backs up the original value.
    // Eventually add to result array.
    if (isset($_SERVER['REMOTE_ADDR'])) {
      $this->request['remoteIP'] = $this->request['updateIP'] = $_SERVER['REMOTE_ADDR'];
    }

    // Determine user from request metadata.
    if (isset($_SERVER['REDIRECT_REMOTE_USER'])) {
      // This happens with mod_cgi or mod_fcgi.
      $this->request['user'] = $_SERVER['REDIRECT_REMOTE_USER'];
    }
    elseif (isset($_SERVER['REMOTE_USER'])) {
      // Standard case with mod_php.
      $this->request['user'] = $_SERVER['REMOTE_USER'];
    }
    elseif (isset($_SERVER['LOGNAME'])) {
      // Bash/CLI call.
      $this->request['user'] = $_SERVER['LOGNAME'];
    }

  }


  /**
   * Generic extension loader.
   *
   * @param string $scope
   *   General extension scope (e. g. "dns", "output", "db").
   * @param string $type
   *   Scope specific sub type (e. g. "generic" for dns, "html" for output).
   * @param string $config
   *   Config section to pass to the interface.
   * @param $a
   *   Optional second argument to pass to the extension constructor.
   *
   * @return object
   *   A created instance of the actual class.
   */
  private function loadExtension($scope, $type, $config, $a = NULL) {

      $classname = sprintf('DDDNS%s%s', $scope, ucfirst($type));
      $classfile = dirname(__FILE__) . "/class.dddns.{$scope}.{$type}.inc";

      require_once $classfile;
      if (!class_exists($classname)) {
        throw new DDDNSexception(sprintf('Required %s interface class "%s" not found.', $scope, $classname));
      }
      if (!$ret = new $classname($config, $a)) {
        throw new DDDNSexception(sprintf('Failed to create %s interface class "%s" instance.', $scope, $classname));
      }

      return $ret;

  }
  /**
   * DNS status cache to minimize system calls.
   *
   * Performs a dig command and caches the result within $data.
   *
   * @param $target
   *   Arbitrary storage name below DDDNS::$data['status'].
   * @param $force
   *   Update a previously cached result in any case.
   *
   * @return DDDNSdnsResult
   *   The dig results related to the $target key.
   */
  private function getDnsStatus($target, $force = FALSE) {
    if ($force || !isset($this->data['status'][$target])) {

      // Prepare results array.
      $res = array(
        'records' => array(),
        'IP' => array(),
      );

      $records = $this->dns->lookup($this->request['host']);
      $res['records'] = $records;

      // Additionally extract A records for more flexibility.
      foreach ($records as $record) {
        if ($record['type'] == 'A') {
          $res['IP'][] = $record['value'];
        }
      }

      $this->data['status'][$target] = $res;
    }
    return $this->data['status'][$target];
  }

  /**
   * Abstraction layer for output formatting.
   *
   * @param string $format
   *   A valid sub-key of $this->config['output']. A corresponding
   *   object $this->output[$format] must have been successfully created
   *   on instantiation.
   *   @see DDDNS::__construct().
   * @param bool $send
   *   "send" parameter for output class.
   *
   * @return mixed
   *   Formatted representation of $data.
   */
  private function formatOutput($format, $send = FALSE) {
    if (!is_object($this->output[$format])) {
      throw new DDDNSexception(sprintf('No valid configuration found for output format "%s".', $format));
    }

    $ret = $this->output[$format]->setHeaders($this->getHeaders())->output($this->result, $send);

    return $ret;
  }

  /**
   * Deletes 'A' records for the current host using the DNS interface.
   *
   * @param $ip
   *   The IP identifying the record to delete. An empty string
   *   means "all".
   *
   * @return bool
   *   TRUE on success.
   */
  private function deleteRecord($ip) {

    // Prepare results array.
    $res = array();

    $this->dns->deleteRecord($this->request['host'], $ip);

    // Check current DNS status.
    $status = $this->getDnsStatus('resulting', TRUE);

    // Verify and return success.
    if (empty($ip)) {
      // "ANY". There must be no more records.
      return empty($status['IP']);
    }
    // Single deletion. $ip must be gone.
    return !in_array($ip, $status['IP']);

  }

  /**
   * Adds an 'A' record for the current host using the DNS interface.
   *
   * @param $ip
   *   The IP/value to add with this record.
   *
   * @return bool
   *   TRUE on success.
   */
  private function addRecord($ip) {

    // Prepare results array.
    $res = array();

    $this->dns->addRecord($this->request['host'], $ip, $this->config['ddns']['ttl']);

    // Verify result vs. current DNS status and return success.
    $status = $this->getDnsStatus('resulting', TRUE);
    return in_array($ip, $status['IP']);
  }

  /**
   * Generic log helper.
   *
   * @param DDDNSresult $result
   *
   * Writes our (or any) DDDNS result to all enabled log backends.
   */
  private function log(DDDNSresult $result = NULL) {
    if (!isset($result)) {
      $result = $this->result;
    }
    foreach($this->log as $type => $log) {
      // Check whether logging is limited for this type and action:
      if (
        empty($this->config['log'][$type]['limitLogEvents'])
        ||
        in_array($result->get('action'), (array) $this->config['log'][$type]['limitLogEvents'])
      ) {
        $log->write($result);
      }
    }
  }

  /**
   * Gets response status and content type from the current internal data.
   */
  public function getHeaders() {
    return array(
      'X-PHP-Response-Code' => $this->httpResultStatus,
    );
  }

  /**
   * Checks whether the current host sent an update within the given timeframe.
   *
   * @param int $lookback
   *   Lookback period in seconds. Defaults to the value passed via
   *   request parameter.
   * @param int|string $logKey
   *   Use the log backend of this name instead of the default one.
   */
  public function check($lookback = NULL, $logKey = '') {

    // Default return value.
    $success = FALSE;

    if (!isset($lookback)) {
      if (empty($this->request['lookback'])) {
        // It actually can (on the logging side), but here it makes no sense.
        throw new DDDNSexception('"Check" method cannot be executed without providing any lookback period.');
      }
      $lookback = $this->request['lookback'];
    }
    if (empty($this->log)) {
      throw new DDDNSexception('The "check" method requires at least one active log backend to be configured. But there is none. Check your config file!');
    }
    if (empty($logKey)) {
      // Set a default value.
      $logKey = array_keys($this->log);
      $logKey = array_shift($logKey);
    }
    if (!isset($this->log[$logKey]) || !is_object($this->log[$logKey])) {
      throw new DDDNSexception(sprintf('The requested log backend "%s" does not exist or is invalid.', $logKey));
    }

    // Retrieve the last record for our host.
    //
    if ($check = $this->log[$logKey]->check($this->request['host'])) {

      // Create shorthand access and provide a readable name.
      $user = $check->get('user');
      $user = empty($user) ? 'unknown' : $user;

      // Add log timestamp to result array.
      // @todo make timestamp format unified and globally configurable.
      $this->result->set('timestamp', $check->get('timestamp'));

      // Compare if value is within limits.
      $lastUpdate = strtotime($check->get('timestamp'));
      $lastAccepted = $this->request['requestTime'] - $lookback;

      // @todo
      // Also make sure the last update matches the current IPs,
      // and there actually ARE Ips, otherwise also notify.
      if ($lastUpdate >= $lastAccepted) {

        // OK.

        // @todo reflect DB IPs and real IPs in result array.

        $success = TRUE;
      }
      else {

        // There has not been any update.
        // Announce this to the recipients passed in with "mailto",
        // unless this has already happened.

        // @todo improve mail tracking (add $force option)

        if (!empty($this->request['cleanup']) && count((array) $this->result->get('previousIP'))) {

          // Prepare a log entry for the deletion and log it.
          $cleanupResult = clone $this->result;
          $cleanupResult
            ->set('clientKey', 'cleanup')
            ->set('clientExtra', sprintf(
              'Check and cleanup triggered by user %s via IP %s at %s.',
              $user,
              $this->request['remoteIP'],
              date(DateTime::RFC1036, $this->request['requestTime'])
            ))
          ;

          // Delete any existing record for this host.
          $cleanupResult->set('success', $this->deleteRecord(''));

          $this->log($cleanupResult);

          // @todo Proper templating!
          $cleanupNote = sprintf(
            "\n\nAlso, a security deletion of all mapped IPs had been requested%s.",
            $cleanupResult->get('success') ?
              ' and successfully executed'
              :
              ', but failed with an error. The IPs are still mapped'
          );

        }
        else {
          $cleanupNote = '';
        }

        $warnCheck = $this->log[$logKey]->check($this->request['host'], array('check'), 'warn');

        // Look up whether a warning has already been sent.
        if (
          // No warning ever has been sent.
          !$warnCheck
          ||
          // A warning HAS been sent but not since the last successful update.
          strtotime($warnCheck->get('timestamp')) < $lastUpdate
        ) {

          // Prepare result and mail data.
          $recipients = explode(';', $this->request['mailto']);

          // @todo: Make configurable.
          $mailSubject = 'WARNING: DDDNS updates';
          $mailBody = sprintf(
            'DDDNS for %s has not been updated for more than %s seconds. The last successful update was at %s from %s (user: %s).%s',
            $check->get('host'),
            $this->request['lookback'],
            $check->get('timestamp'),
            $check->get('remoteIP'),
            $user,
            $cleanupNote
          );

          $mailSuccess = TRUE;

          foreach ($recipients as $recipient) {
            // Send out mail in any case, but track success.
            $mailSuccess = mail(
              $recipient,
              $mailSubject,
              $mailBody,
              sprintf('from: DDDNScheck@%s', $check->get('host'))
            ) && $mailSuccess;
          }

          // Prepare a log entry for the warning and log it.
          $warnResult = clone $this->result;
          $warnResult->set('clientKey', 'warn')->set('success', $mailSuccess);
          $this->log($warnResult);

        }


      }
    }
    else {
      throw new DDDNSexception(sprintf('There is no update or add entry for "%s" ever. Are you sure this host triggers dynamic updates?', $this->request['host']));
    }

    $this->httpResultStatus = $success ? DDDNS::HTTP_STATUS_OK : DDDNS::HTTP_STATUS_NOT_FOUND;

    return $success;

  }

  /**
   * Performs (an) nsupdate(s) for each requested host and returns the result(s).
   *
   * @param string|array $ips
   *   (Optional) one or multiple ips to explicitely delete.
   *   Defaults to the IP(s) provided in the "deleteIP" request
   *   parameter which must be set.
   *
   * @return array
   *   An array with both the success flag and the "update needed tracker".
   */
  public function update($ips = NULL) {

    if (!isset($ips)) {
      $ips = $this->request['updateIP'];
    }

    $ips = (array) $ips;

    // Do not run if no new IPs are given.
    if (!empty($ips)) {

      // Check existing IP entries (already cached in DDDNS::__construct()).
      $status = $this->getDnsStatus('initial');
      $existing = $status['IP'];

      // Determine necessary actions:
      $outdated = array_diff($existing, $ips);
      $missing = array_diff($ips, $existing);

      // Decide if an update is required and eventually try to perform it.
      // We will update if either one record with a different IP or
      // no record with the current IP is found.
      $res['needUpdate'] = !empty($outdated) || !empty($missing);

      if ($res['needUpdate']) {

        // First, add the new IPs (to avoid "empty" periods).
        $res['success'] = (empty($missing) || $this->add($missing));

        // Deletion will only be performed if adding succeeded.
        // If you need an unconditional delete, explicitely trigger
        // DDDNS::delete().
        $res['success'] = $res['success'] && (empty($outdated) || $this->delete($outdated));

      }

      // Eventually log results to the database.
      // Load recent DB entry for $this->request['host'].
      // format_result() according to the demanded result type and set headers.
      // return success.
    }
    else {
      throw new DDDNSexception(
        'Update not allowed: No IP has been provided.',
        DDDNS::HTTP_STATUS_BAD_REQUEST
      );
    }

    // Set http status code.
    $this->httpResultStatus = isset($res['success']) ?
      (
        $res['success'] ?
          DDDNS::HTTP_STATUS_OK
          :
          DDDNS::HTTP_STATUS_INTERNAL_SERVER_ERROR
      )
      :
      // @todo re-evaluate.
      // This makes sure a HTTP 200 is thrown even if no action was necessary.
      // The former DDDNS::HTTP_STATUS_NOT_MODIFIED was syntactically right and
      // also respected HTTP status definitions, yet confused browsers.
      (
        $res['needUpdate'] ?
          (
            // Config value is accepted to be NULL, which here equals FALSE.
            TRUE === @$this->request['allow_http_not_modified'] ?
              DDDNS::HTTP_STATUS_NOT_MODIFIED
              :
              DDDNS::HTTP_STATUS_OK
          )
          :
          DDDNS::HTTP_STATUS_INTERNAL_SERVER_ERROR
      )
    ;
    return $res;

  }

  /**
   * Public interface to DDDNS::deleteRecord().
   *
   * @param string|array $ips
   *   (Optional) one or multiple ips to explicitely delete.
   *   Defaults to the IP(s) provided in the "deleteIP" request
   *   parameter which must be set.
   */
  public function delete($ips = NULL) {
    if (!isset($ips)) {
      $ips = empty($this->request['deleteIP']) ? '' : $this->request['deleteIP'];
    }

    $success = TRUE;
    foreach((array) $ips as $ip) {
      $success = $success && $this->deleteRecord($ip);
    }

    // Set http status code according to task and success.
    $this->httpResultStatus = isset($success) ?
      ($success ? DDDNS::HTTP_STATUS_GONE : DDDNS::HTTP_STATUS_INTERNAL_SERVER_ERROR)
      :
      DDDNS::HTTP_STATUS_NOT_MODIFIED
    ;

    return $success;
  }

  /**
   * Public interface to DDDNS::addRecord().
   *
   * @param string|array $ips
   *   (Optional) one or multiple ips to explicitely add.
   *   Defaults to the IP provided in the "ip" request
   *   parameter (resp. the one read from the request).
   */
  public function add($ips = NULL) {

    if (!isset($ips)) {
      $ips = $this->request['updateIP'];
    }

    $success = TRUE;
    foreach((array) $ips as $ip) {
      $success = $success && $this->addRecord($ip);
    }

    // Set http status code according to task and success.
    $this->httpResultStatus = $success ?
      (empty($ips) ? DDDNS::HTTP_STATUS_OK : DDDNS::HTTP_STATUS_CREATED)
      :
      DDDNS::HTTP_STATUS_INTERNAL_SERVER_ERROR
    ;

    return $success;
  }

  /**
   * Returns the current DNS records for $this->host.
   *
   * @param bool $force
   *   If TRUE, a refresh is forced. Otherwise the result may be
   *   taken from cache.
   */
  public function lookup($force = FALSE) {
    return $this->getDnsStatus('lookup', $force);
  }

  /**
   * Returns an HTTP status code representation of the performed action, if any.
   *
   * @return int
   *   One of the DDDNS::HTTP_STATUS constants.
   */
  public function getResultStatusCode() {
    return $this->httpResultStatus;
  }

  /**
   * Executes the defined action and returns the result in the requested format.
   *
   * @return object
   *   The current DDDNS object for chaining.
   */
  public function execute() {

    // Get the previous host status.
    $dnsold = $this->getDnsStatus('initial');

    // Prepare action results array and execute non-read actions.
    $res = array();
    switch ($this->request['action']) {
      case 'update':
        $res = $this->update();
        extract($res);
        break;
      case 'delete':
        $success = $this->delete();
        break;
      case 'add':
        $success = $this->add();
        break;
      case 'check':
        $success = $this->check();
        break;
      default:
        $success = !empty($dnsold['IP']);
        $this->httpResultStatus = $success ?
          DDDNS::HTTP_STATUS_OK
          :
          DDDNS::HTTP_STATUS_NOT_FOUND
        ;
    }

    // Get the current host status after the action and add to results.
    $this->result
      ->setDns('current', ($dnsresult = $this->getDnsStatus('resulting')))
      ->set('success', @$success)
      ->set('needUpdate', @$needUpdate)
    ;

    $this->log();

    return $this;

  }

  /**
   * Getter for the $this->result object.
   *
   * @return DDDNSresult
   *   The result object reflecting all previously performed actions, if any.
   */
  public function getResult() {

    return $this->result;

  }

  /**
   * Returns the current output through a selected/configured output extension.
   *
   * This wrapper is primarily to allow for (limited) client-side
   * data format selection and usage in wrappers like "index.php".
   * Programmatically, you may also consider the public $this->output array,
   * containing all configured extensions and offering all their public methods
   * for direct access.
   *
   * @param string $outputType
   *   Must match an existing output extension ("html", "json" or any custom).
   *   Defaults to "html", overridden by an optional request argument.
   * @param bool $send
   *   Whether or not to send the parsed output.
   *
   * @return mixed
   *   The current DDDNS result, parsed by the output extension
   *   and optionally already sent to the client.
   */
  public function getOutput($outputType = NULL, $send = FALSE) {

    // Set fallback/default if required.
    if (empty($outputType)) {
      $outputType = !empty($this->request['output']) ? $this->request['output'] : 'html';
    }

    return $this->formatOutput($outputType, $send);

  }

  /**
   * Shorthand function for getOutput('', TRUE).
   */
  public function sendOutput($outputType = NULL) {
    return $this->getOutput($outputType, TRUE);
  }

  public function mailDummy() {
  }

  /**
   * Calls installer hooks for all configured log extensions.
   *
   * @return object
   *   $this for chaining.
   */
  public function install() {
    $res = array();
    foreach ($this->log as $type => $log) {
      try {
        $log->install();
        $res[$type] = TRUE;
      }
      catch (DDDNSException $e) {
        $this->httpResultStatus = $e->getStatusCode();
        $res[$type] = $e->getMessage();
      }
    }
    $this->result->set('success', $res);
    return $this;
  }

}
