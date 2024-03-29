--TEST--
Custom save handler, multiple session_start()s, complex data structure test.
--FILE--
<?php

error_reporting(E_ALL & ~E_NOTICE);

class handler {
	var $data = 'baz|O:3:"foo":2:{s:3:"bar";s:2:"ok";s:3:"yes";i:1;}arr|a:1:{i:3;O:3:"foo":2:{s:3:"bar";s:2:"ok";s:3:"yes";i:1;}}';
    function open($save_path, $session_name)
    {
        print "OPEN: $save_path, $session_name\n";
        return true;
    }
    function close()
    {
		print "CLOSE\n";
        return true;
    }
    function read($key)
    {
        print "READ: $key\n";
        return $GLOBALS["hnd"]->data;
    }

    function write($key, $val)
    {
        print "WRITE: $key, $val\n";
		$GLOBALS["hnd"]->data = $val;
        return true;
    }

    function destroy($key)
    {
        print "DESTROY: $key\n";
        return true;
    }

    function gc() { return true; }
}

$hnd = new handler;

class foo {
    var $bar = "ok";
    function method() { $this->yes++; }
}

ob_start();

session_set_save_handler(array($hnd, "open"), array($hnd, "close"), array($hnd, "read"), array($hnd, "write"), array($hnd, "destroy"), array($hnd, "gc"));

session_id("test");
session_start();
$baz->method();
$arr[3]->method();

var_dump($baz);
var_dump($arr);

session_write_close();

session_set_save_handler(array($hnd, "open"), array($hnd, "close"), array($hnd, "read"), array($hnd, "write"), array($hnd, "destroy"), array($hnd, "gc"));
session_start();
$baz->method();
$arr[3]->method();


$c = 123;
session_register("c");
var_dump($baz); var_dump($arr); var_dump($c);

session_write_close();

session_set_save_handler(array($hnd, "open"), array($hnd, "close"), array($hnd, "read"), array($hnd, "write"), array($hnd, "destroy"), array($hnd, "gc"));
session_start();
var_dump($baz); var_dump($arr); var_dump($c);

session_destroy();
?>
--EXPECT--
OPEN: /tmp, PHPSESSID
READ: test
object(foo)(2) {
  ["bar"]=>
  string(2) "ok"
  ["yes"]=>
  int(2)
}
array(1) {
  [3]=>
  &object(foo)(2) {
    ["bar"]=>
    string(2) "ok"
    ["yes"]=>
    int(2)
  }
}
WRITE: test, baz|O:3:"foo":2:{s:3:"bar";s:2:"ok";s:3:"yes";i:2;}arr|a:1:{i:3;O:3:"foo":2:{s:3:"bar";s:2:"ok";s:3:"yes";i:2;}}
CLOSE
OPEN: /tmp, PHPSESSID
READ: test
object(foo)(2) {
  ["bar"]=>
  string(2) "ok"
  ["yes"]=>
  int(3)
}
array(1) {
  [3]=>
  &object(foo)(2) {
    ["bar"]=>
    string(2) "ok"
    ["yes"]=>
    int(3)
  }
}
int(123)
WRITE: test, baz|O:3:"foo":2:{s:3:"bar";s:2:"ok";s:3:"yes";i:3;}arr|a:1:{i:3;O:3:"foo":2:{s:3:"bar";s:2:"ok";s:3:"yes";i:3;}}c|i:123;
CLOSE
OPEN: /tmp, PHPSESSID
READ: test
object(foo)(2) {
  ["bar"]=>
  string(2) "ok"
  ["yes"]=>
  int(3)
}
array(1) {
  [3]=>
  object(foo)(2) {
    ["bar"]=>
    string(2) "ok"
    ["yes"]=>
    int(3)
  }
}
int(123)
DESTROY: test
CLOSE
