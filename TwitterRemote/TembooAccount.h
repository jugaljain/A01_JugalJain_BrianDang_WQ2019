/*
* IMPORTANT NOTE about TembooAccount.h
*
* TembooAccount.h contains your Temboo account information and must be included
* alongside TembooMain.c. 
*/

#ifndef TEMBOOACCOUNT_H_
#define TEMBOOACCOUNT_H_

#define TEMBOO_ACCOUNT "jugaljain"  // Your Temboo account name
#define TEMBOO_APP_KEY_NAME "myFirstApp"  // Your Temboo app name
#define TEMBOO_APP_KEY "pqc6xUxspMkqh52gA3pbsCyXVofZjbMW"  // Your Temboo app key
#define TEMBOO_DEVICE_TYPE "CC3220S"

#define WIFI_SSID "Crapfinity"
#define WPA_PASSWORD "Godzilla310"
#define WIFI_SECURITY_TYPE SL_WLAN_SEC_TYPE_WPA_WPA2

#endif /* TEMBOOACCOUNT_H_ */

/* 
* The same TembooAccount.h file settings can be used for all of your Temboo programs.  
* Keeping your account information in a separate file means you can share the 
* main.c file without worrying that you forgot to delete your credentials.
*/