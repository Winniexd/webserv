#!/bin/sh

# Set the Content-Type to indicate this is HTML output
echo "Content-Type: text/html"
echo ""

# HTML response
echo "<html>"
echo "<head><title>CGI Example</title></head>"
echo "<body>"
echo "<h1>Hello from Bash CGI!</h1>"
echo "<p>Current date and time: $(date)</p>"

# Example: Display query string parameters (if any)
echo "<h2>Query Parameters:</h2>"
echo "<p>QUERY_STRING: $QUERY_STRING</p>"

# Example: Display environment variables
echo "<h2>Environment Variables:</h2>"
echo "<pre>"
env
echo "</pre>"

echo "</body>"
echo "</html>"
