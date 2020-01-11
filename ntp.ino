// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty

#include <NtpClientLib.h>

// ************************************
// processSyncEvent()
//
// manage NTP sync events and warn in case of error
// ************************************
void processSyncEvent(NTPSyncEvent_t ntpEvent) {
  DEBUG("[DEBUG] processSyncEvent() ");
  if (ntpEvent) {
    DEBUG("[ERROR] Time Sync error: ");
    if (ntpEvent == noResponse)
      DEBUG("[ERROR] NTP server not reachable");
    else if (ntpEvent == invalidAddress)
      DEBUG("[ERROR] Invalid NTP server address");
  } else {
    DEBUG("[DEBUG] Got NTP time: "+NTP.getTimeDateString(NTP.getLastNTPSync ()));
  }
}
