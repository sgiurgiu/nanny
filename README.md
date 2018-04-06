Web Nanny
================
Control access to the internet for the specified hosts. It allows those hosts to give themselves acess/revoke it via a simple web interface.

The way it works is this:
The firewall is configured to either deny or allow access to the external interface for the IPs in a certain table. 
This tool simply add/removes IPs in the configured table depending on a flag. If the firewall is configured to block evreything, 
but only allow addresses in a table, then this tool can add an IP to the table when allowing the host, and removing otherwise.

It allows for daily (as in the days of the week) control of the number of minutes allowed. A host, when it wants internet access it visits the website and it gives itself access. The website has to be available on the internal LAN for it to be accessible.
When it is done with the internet for now, simply visits the website and it stops access (stops the counter). If the number of minutes expires, access is stopped automatically.

At the moment there is no configuration page (would imply admin access, which implies authentication) so all configuration is done in the database directly.


