#!/bin/bash

var1=`echo $QUERY_STRING | awk -F = '{ print $2 }' | awk '{gsub("%2F", "/", $0); print}'`

printf "Content-type: text/html\r\n"
printf "\r\n"
printf "<html>\n"
printf "<head>\n"
printf "<title>My first page!</title>\n"
printf "</head>\n"
printf "<body>\n"
printf "<p>\n"
cat $var1 | awk '{ print "<p>"$n"</p>" }'
printf "</p>\n"
printf "</body>\n"
printf "</html>\n"
