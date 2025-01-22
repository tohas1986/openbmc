Our patch set to Intel-ipmi-oem contains following:
1. getDeviceId: Parse our version of os-release
2. "no reading": Return correct "no reading" of sensors with NULL value
3. AC Fail status: Get rid of ACFail status that doesn't exist in our servers
4. Disable setSelTime: Not sure, probably it's not used as we use NTP
5. Add addSelEntry command (intel-ipmi-oem only adds to RedFish logging system, we add it to IPMI SEL)
6. Reduce logging, change INFO to DEBUG in many places. TBD: join all that patches to single one
