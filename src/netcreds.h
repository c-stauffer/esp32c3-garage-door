#ifndef _NETCREDS_H
#define _NETCREDS_H

// You'll need to edit this with actual values.
const char* NC_SSID = "redacted";
const char* NC_PASSWORD = "redacted";

const int NC_LISTEN_PORT = 0;

// specify a static IP for the micro and other
// relevant info. In this case the static IP will be 192.168.1.42
static IPAddress NC_MY_IP(192, 168, 1, 45);
static IPAddress NC_GATEWAY(192, 168, 1, 1);
static IPAddress NC_SUBNET(255, 255, 0, 0);

#endif
