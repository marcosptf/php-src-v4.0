<?php
//
// +----------------------------------------------------------------------+
// | PHP version 4.0                                                      |
// +----------------------------------------------------------------------+
// | Copyright (c) 1997-2001 The PHP Group                                |
// +----------------------------------------------------------------------+
// | This source file is subject to version 2.02 of the PHP license,      |
// | that is bundled with this package in the file LICENSE, and is        |
// | available at through the world-wide-web at                           |
// | http://www.php.net/license/2_02.txt.                                 |
// | If you did not receive a copy of the PHP license and are unable to   |
// | obtain it through the world-wide-web, please send a note to          |
// | license@php.net so we can mail you a copy immediately.               |
// +----------------------------------------------------------------------+
// | Authors: Sterling Hughes <sterling@php.net>                          |
// +----------------------------------------------------------------------+
//
// $Id: msql.php,v 1.19 2001/08/07 11:55:55 cox Exp $
//
// Database independent query interface definition for PHP's Mini-SQL
// extension.
//

require_once 'DB/common.php';

class DB_msql extends DB_common
{
    var $connection;
    var $phptype, $dbsyntax;
    var $prepare_tokens = array();
    var $prepare_types = array();

    function DB_msql()
    {
        $this->DB_common();
        $this->phptype = 'msql';
        $this->dbsyntax = 'msql';
        $this->features = array(
            'prepare' => false,
            'pconnect' => true,
            'transactions' => false
        );
    }

    function connect($dsninfo, $persistent = false)
    {
        $this->dsn = $dsninfo;
        $user = $dsninfo['username'];
        $pw = $dsninfo['password'];
        $dbhost = $dsninfo['hostspec'] ? $dsninfo['hostspec'] : 'localhost';
        $connect_function = $persistent ? 'msql_pconnect' : 'msql_connect';
        if ($dbhost && $user && $pw) {
            $conn = $connect_function($dbhost, $user, $pw);
        } elseif ($dbhost && $user) {
            $conn = $connect_function($dbhost,$user);
        } else {
            $conn = $connect_function($dbhost);
        }
        if (!$conn) {
            $this->raiseError(DB_ERROR_CONNECT_FAILED);
        }
        if (!@msql_select_db($dsninfo['database'], $conn)){
            return $this->raiseError(DB_ERROR_NODBSELECTED);
        }
        $this->connection = $conn;
        return DB_OK;
    }

    function disconnect()
    {
        $ret = @msql_close($this->connection);
        $this->connection = null;
        return $ret;
    }

    function simpleQuery($query)
    {
        $this->last_query = $query;
        $query = $this->modifyQuery($query);
        $result = @msql_query($query, $this->connection);
        if (!$result) {
            return $this->raiseError();
        }
        // Determine which queries that should return data, and which
        // should return an error code only.
        return DB::isManip($query) ? DB_OK : $result;
    }

    function fetchRow($result, $fetchmode = DB_FETCHMODE_DEFAULT, $rownum=null)
    {
        if ($fetchmode == DB_FETCHMODE_DEFAULT) {
            $fetchmode = $this->fetchmode;
        }
        $res = $this->fetchInto ($result, $arr, $fetchmode, $rownum);
        if ($res !== DB_OK) {
            return $res;
        }
        return $arr;
    }

    function fetchInto($result, &$ar, $fetchmode, $rownum=null)
    {
        if ($rownum !== null) {
            if (!@msql_data_seek($result, $rownum)) {
                return null;
            }
        }
        if ($fetchmode & DB_FETCHMODE_ASSOC) {
            $ar = @msql_fetch_array($result, MSQL_ASSOC);
        } else {
            $ar = @msql_fetch_row($result);
        }
        if (!$ar) {
            if ($error = msql_error()) {
                return $this->raiseError($error);
            } else {
                return null;
            }
        }
        return DB_OK;
    }

    function freeResult($result)
    {
        if (is_resource($result)) {
            return @msql_free_result($result);
        }
        if (!isset($this->prepare_tokens[$result])) {
            return false;
        }
        unset($this->prepare_tokens[$result]);
        unset($this->prepare_types[$result]);
        return true;
    }

    function numCols($result)
    {
        $cols = @msql_num_fields($result);
        if (!$cols) {
            return $this->raiseError();
        }
        return $cols;
    }

    function numRows($result)
    {
        $rows = @msql_num_rows($result);
        if (!$rows) {
            return $this->raiseError();
        }
        return $rows;
    }

    /**
     * Gets the number of rows affected by a query.
     *
     * @return number of rows affected by the last query
     */

    function affectedRows()
    {
        return @msql_affected_rows($this->connection);
    }

}
?>