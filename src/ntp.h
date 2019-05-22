#ifndef _NtpTime_H
#define _NtpTime_H
#include <Arduino.h>
#include <WiFiUdp.h>
#ifdef ARDUNO_ARCH_ESP8266
#include <ESP8266WiFi.h>
#elif ARDUNO_ARCH_ESP32
#include <WiFi.h>
#endif

#define NtpTime_DEFAULT_PORT      123
#define NtpTime_DEFAULT_SERVER    "time.nist.gov"
#define NTP_PACKET_SIZE             48
#define NtpTime_DEFAULT_TIMEOUT   5000
#define NTP_DEFAULT_TIMEZONE        7

class NtpTime{
    public :
        NtpTime();
        NtpTime(const char * ntpServer,int localport=NtpTime_DEFAULT_PORT);
        void begin();
        time_t getTime(int timezone=NTP_DEFAULT_TIMEZONE,int timeout=NtpTime_DEFAULT_TIMEOUT);
    private:
        boolean sendNTPpacket ();
        char _ntpServer[100];
        int _ntpLocalPort;
        WiFiUDP *udp;
        int timeZone=NTP_DEFAULT_TIMEZONE;
};

NtpTime::NtpTime(const char*server,int localPort):
    _ntpLocalPort(localPort)
{
    strncpy(_ntpServer,server,sizeof(_ntpServer));
}
NtpTime::NtpTime():NtpTime(NtpTime_DEFAULT_SERVER,NtpTime_DEFAULT_PORT){}

void NtpTime::begin(){
    udp = new WiFiUDP ();
} 

boolean NtpTime::sendNTPpacket () {
    uint8_t ntpPacketBuffer[NTP_PACKET_SIZE]; //Buffer to store request message
                                           // set all bytes in the buffer to 0
    memset (ntpPacketBuffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    ntpPacketBuffer[0] = 0b11100011;   // LI, Version, Mode
    ntpPacketBuffer[1] = 0;     // Stratum, or type of clock
    ntpPacketBuffer[2] = 6;     // Polling Interval
    ntpPacketBuffer[3] = 0xEC;  // Peer Clock Precision
                                // 8 bytes of zero for Root Delay & Root Dispersion
    ntpPacketBuffer[12] = 49;
    ntpPacketBuffer[13] = 0x4E;
    ntpPacketBuffer[14] = 49;
    ntpPacketBuffer[15] = 52;
    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:

    udp->beginPacket (_ntpServer, _ntpLocalPort); //NTP requests are to port 123
    udp->write (ntpPacketBuffer, NTP_PACKET_SIZE);
    udp->endPacket ();    
    return true;
}

time_t NtpTime::getTime(int timezone,int timeOut){
    byte packetBuffer[NTP_PACKET_SIZE]; //Buffer to store response message
    udp->begin(_ntpLocalPort);
    while (udp->parsePacket () > 0);// discard any previously received packets
    sendNTPpacket();
    uint32_t beginWait = millis ();

    while ((millis () - beginWait) < timeOut) {
        int size = udp->parsePacket ();
        if (size >= NTP_PACKET_SIZE) {
            udp->read(packetBuffer, NTP_PACKET_SIZE);            
            unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
            unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
            // combine the four bytes (two words) into a long integer
            // this is NTP time (seconds since Jan 1 1900):
            unsigned long secsSince1900 = highWord << 16 | lowWord;
            const unsigned long seventyYears = 2208988800UL;
            // subtract seventy years:
            unsigned long epoch = (secsSince1900 - seventyYears)+(timezone * 3600);                        
            return (time_t)epoch;
        }        
        delay(100);
    }
    udp->stop();
}
#endif
