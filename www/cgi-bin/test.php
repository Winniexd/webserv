<?php
     header('Content-Type: text/plain');
     echo "Test CGI PHP
";
     echo "REQUEST_METHOD: " . $_SERVER['REQUEST_METHOD'] . "
";
     echo "QUERY_STRING: " . $_SERVER['QUERY_STRING'] . "
";
     if ($_SERVER['REQUEST_METHOD'] === 'POST') {
         echo "POST data: " . file_get_contents('php://input') . "
";
     }
     ?>
