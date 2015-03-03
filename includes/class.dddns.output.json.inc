<?php

/**
 * @file
 * Extension to output formatted HTML.
 *
 * Requires the following extension structures in the global DDDNS config array:
 *
 * $global_config = array(
 *   'output' => array(
 *     'json' => array(
 *       // The following uses the formatter class
 *       //   "./DDDNS.output.json.inc"
 *       'formatter' => 'default',
 *     ),
 *   ),
 * );
 *
 */

require_once dirname(__FILE__) . '/class.dddns.output.inc';

class DDDNSoutputJson extends DDDNSoutput {

  function __construct($config, $headers) {

    // Override content type header.
    $headers['Content-type'] = 'text/javascript';

    parent::__construct($config, $headers);

  }

  /**
   * Parses a DDDNSresult object into JSON.
   *
   * @param DDDNSresult $resultObject
   *   DDDNS Result object.
   * @param bool $send
   *   (Optional) Whether to send the output directly to stdout.
   *   Defaults to FALSE.
   *
   * @return string
   *   The parsed markup.
   */
  public function output(DDDNSresult $resultObject, $send = FALSE) {

    $ret = json_encode($resultObject->get($this->outputFields));

    if ($send) {
      $this->sendHeaders();
      echo $ret;
    }

    return $ret;

  }

}
