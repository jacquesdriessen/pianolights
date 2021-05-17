// V1.03 Stable version.
// V1.02 Clean up a bit
// V1.01 Second attempt to scan for apple-midi devices
// V1.00 First production release
// V0.07 Attempt to connect to apple-midi devices on network don't seem to resolve mdns, anyway now hardcoded connect to ipad
// V0.06 Use table for colours
// V0.05 Velocity determines brightness
// V0.04 Add ability to save/recover/set password (through web / ap mode).
// V0.03 Add MDNS discovery
// V0.02 Integrate applemidi
// V0.01 First version

// Includes

extern "C" {
#include "user_interface.h"
}
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <mdns.h>
#include <EEPROM.h>
#include "AppleMidi.h"
#include <NeoPixelBus.h>

bool isConnected = false;
APPLEMIDI_CREATE_INSTANCE(WiFiUDP, AppleMIDI); // see definition in AppleMidi_Defs.h

#define QUESTION_SERVICE "_apple-midi._udp.local"
#define MAX_HOSTS 4
#define HOSTS_SERVICE_NAME 0
#define HOSTS_PORT 1
#define HOSTS_HOST_NAME 2
#define HOSTS_ADDRESS 3
String hosts[MAX_HOSTS][4];  // Array containing information about hosts received over mDNS.

#define MIDDLE_C 60
#define keysCount (72)
#define pixelCount (keysCount*2)
NeoPixelBus strip = NeoPixelBus(pixelCount, 2, NEO_GRB);

float keyrColor[12] = { 1.00, 1.00, 1.00, 1.00, 0.50, 0.00, 0.00, 0.00, 0.00, 0.75, 1.00, 1.00 };
float keygColor[12] = { 0.00, 0.50, 0.75, 1.00, 1.00, 1.00, 1.00, 1.00, 0.00, 0.00, 0.00, 0.00 };
float keybColor[12] = { 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.75, 1.00, 1.00, 1.00, 1.00, 0.50 };

int r = 0;
int g = 0;
int b = 0;

struct Pixel {
  byte r = 0;
  byte g = 0;
  byte b = 0;
};

struct Pixel hsv2rgb(double h, double s, double v) {
    struct Pixel pixel;
    double r, g, b;

    int i = int(h * 6);
    double f = h * 6 - i;
    double p = v * (1 - s);
    double q = v * (1 - f * s);
    double t = v * (1 - (1 - f) * s);

    switch(i % 6){
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }
    pixel.r = 255*r;
    pixel.g = 255*g;
    pixel.b = 255*b;
    
    return pixel;
}

// ====================================================================================
// Event handlers for incoming MIDI messages
// ====================================================================================

// -----------------------------------------------------------------------------
// rtpMIDI session. Device connected
// -----------------------------------------------------------------------------
void OnAppleMidiConnected(uint32_t ssrc, char* name) {
  Serial.print(name);
  Serial.println(" connected");
  isConnected  = true;
  // colours OFF
  for (int i = 0; i < 144; i++)
    strip.SetPixelColor(i, RgbColor(0,0,0));
  strip.Show();    
}

// -----------------------------------------------------------------------------
// rtpMIDI session. Device disconnected
// -----------------------------------------------------------------------------
void OnAppleMidiDisconnected(uint32_t ssrc) {
  isConnected  = false;
  Serial.println("Disconnect");
}

byte NoteToLight(byte note) {
  int light = keysCount + 2 * (note - MIDDLE_C);
  if (note < MIDDLE_C)
    light += 1;
  if ((light < 0) || (light > 144))
    light = 255;
  return (byte) light;
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void OnAppleMidiNoteOn(byte channel, byte note, byte velocity) {
  int offset = constrain(note%12,0,12);
  byte light = NoteToLight(note);
  if (light != 255) {
    strip.SetPixelColor(light , RgbColor(velocity*keyrColor[offset],velocity*keygColor[offset],velocity*keybColor[offset]));
    strip.Show();
  }
//  Serial.println("Note On");
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void OnAppleMidiNoteOff(byte channel, byte note, byte velocity) {
  byte light = NoteToLight(note);
  if (light != 255) {
    strip.SetPixelColor(light, RgbColor(0,0,0));
    strip.Show();
  }
//  Serial.println("Note Off");
}

void parseBytes(const char* str, char sep, byte* bytes, int maxBytes, int base) {
    for (int i = 0; i < maxBytes; i++) {
        bytes[i] = strtoul(str, NULL, base);  // Convert byte
        str = strchr(str, sep);               // Find next separator
        if (str == NULL || *str == '\0') {
            break;                            // No more separators, exit
        }
        str++;                                // Point to next character after separator
    }
}

void answerCallback(const mdns::Answer* answer) {

  // A typical PTR record matches service to a human readable name.
  // eg:
  //  service: _mqtt._tcp.local
  //  name:    Mosquitto MQTT server on twinkle.local
  if (answer->rrtype == MDNS_TYPE_PTR and strstr(answer->name_buffer, QUESTION_SERVICE) != 0) {
    unsigned int i = 0;
    for (; i < MAX_HOSTS; ++i) {
      if (hosts[i][HOSTS_SERVICE_NAME] == answer->rdata_buffer) {
        // Already in hosts[][].
        break;
      }
      if (hosts[i][HOSTS_SERVICE_NAME] == "") {
        // This hosts[][] entry is still empty.
        hosts[i][HOSTS_SERVICE_NAME] = answer->rdata_buffer;
        break;
      }
    }
    if (i == MAX_HOSTS) {
      Serial.print(" ** ERROR ** No space in buffer for ");
      Serial.print('"');
      Serial.print(answer->name_buffer);
      Serial.print('"');
      Serial.print("  :  ");
      Serial.print('"');
      Serial.println(answer->rdata_buffer);
      Serial.print('"');
    }
  }

  // A typical SRV record matches a human readable name to port and FQDN info.
  // eg:
  //  name:    Mosquitto MQTT server on twinkle.local
  //  data:    p=0;w=0;port=1883;host=twinkle.local
  if (answer->rrtype == MDNS_TYPE_SRV) {
    unsigned int i = 0;
    for (; i < MAX_HOSTS; ++i) {
      if (hosts[i][HOSTS_SERVICE_NAME] == answer->name_buffer) {
        // This hosts entry matches the name of the host we are looking for
        // so parse data for port and hostname.
        char* port_start = strstr(answer->rdata_buffer, "port=");
        if (port_start) {
          port_start += 5;
          char* port_end = strchr(port_start, ';');
          char port[1 + port_end - port_start];
          strncpy(port, port_start, port_end - port_start);
          port[port_end - port_start] = '\0';

          if (port_end) {
            char* host_start = strstr(port_end, "host=");
            if (host_start) {
              host_start += 5;
              hosts[i][HOSTS_PORT] = port;
              hosts[i][HOSTS_HOST_NAME] = host_start;
            }
          }
        }
        break;
      }
    }
    if (i == MAX_HOSTS) {
      Serial.print(" Did not find ");
      Serial.print('"');
      Serial.print(answer->name_buffer);
      Serial.print('"');
      Serial.println(" in hosts buffer.");
    }
  }

  // A typical A record matches an FQDN to network ipv4 address.
  // eg:
  //   name:    twinkle.local
  //   address: 192.168.192.9
  if (answer->rrtype == MDNS_TYPE_A) {
    int i = 0;
    for (; i < MAX_HOSTS; ++i) {
      if (hosts[i][HOSTS_HOST_NAME] == answer->name_buffer) {
        hosts[i][HOSTS_ADDRESS] = answer->rdata_buffer;
        break;
      }
    }
    if (i == MAX_HOSTS) {
      Serial.print(" Did not find ");
      Serial.print('"');
      Serial.print(answer->name_buffer);
      Serial.print('"');
      Serial.println(" in hosts buffer.");
    }
  }

  Serial.println();
  for (int i = 0; i < MAX_HOSTS; ++i) {
    if (hosts[i][HOSTS_SERVICE_NAME] != "") {
      Serial.print(">  ");
      Serial.print(hosts[i][HOSTS_SERVICE_NAME]);
      Serial.print("    ");
      Serial.print(hosts[i][HOSTS_PORT]);
      Serial.print("    ");
      Serial.print(hosts[i][HOSTS_HOST_NAME]);
      Serial.print("    ");
      Serial.println(hosts[i][HOSTS_ADDRESS]);

      byte ip[4];
      parseBytes(hosts[i][HOSTS_ADDRESS].c_str(), '.', ip, 4, 10);
      AppleMIDI.invite(IPAddress(ip[0],ip[1],ip[2],ip[3]));
    }
  }
}

mdns::MDns my_mdns(NULL, NULL, answerCallback);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(76800);
  
  Serial.println("All LEDs green");
  // colours green
  for (int key = 0; key < keysCount; key++)
  {
    for (int i = 0; i < 2; i++)
      strip.SetPixelColor(i + (2*key), RgbColor(0,128,0));
  }
  strip.Show();

  EEPROM.begin(512);
  delay(10);

  String esid = "";
  for (int i = 0; i < 32; ++i)
    {
      esid += char(EEPROM.read(i));
    }
  String epass = "";
  for (int i = 32; i < 96; ++i)
    {
      epass += char(EEPROM.read(i));
    }

  Serial.println("Connecting to previous access point");
  
  WiFi.begin(esid.c_str(), epass.c_str());

  int wificonnection = 144;
  while ((WiFi.status() != WL_CONNECTED) && wificonnection > 0) {
    delay(50);
    Serial.print(".");
    wificonnection --;
    // shift to blue
    strip.SetPixelColor(wificonnection, RgbColor(0,0,128));
    strip.Show();        
  }
  Serial.println();
  
  if (wificonnection == 0) {
    Serial.println("Starting web server to enter new wifi details");
    Serial.println("All LEDs green");
    // colours red
    for (int i = 0; i < 144; i++)
      strip.SetPixelColor(i, RgbColor(128,0,0));
    strip.Show();    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    int n = WiFi.scanNetworks();
    String st = "<ul>";
    for (int i = 0; i < n; ++i)
      {
        // Print SSID and RSSI for each network found
        st += "<li>";
        st +=i + 1;
        st += ": ";
        st += WiFi.SSID(i);
        st += " (";
        st += WiFi.RSSI(i);
        st += ")";
        st += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
        st += "</li>";
      }
    st += "</ul>";
    delay(100);
    WiFi.softAP("piano", "");    

    WiFiServer server(80); 
    
    server.begin();
               
    while (1) {
      WiFiClient client = server.available();
      if (client) {
        while(client.connected() && !client.available()) {
          delay(1);
        }
        String req = client.readStringUntil('\r');
        int addr_start = req.indexOf(' ');
        int addr_end = req.indexOf(' ', addr_start + 1);
        if (addr_start != -1 && addr_end != -1) {
          req = req.substring(addr_start + 1, addr_end);
          client.flush(); 
          String s;   
          if (req == "/") {
            IPAddress ip = WiFi.softAPIP();
            String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
            s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Piano at ";
            s += ipStr;
            s += "<p>";
            s += st;
            s += "<form method='get' action='a'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form>";
            s += "</html>\r\n\r\n";
          }
          else if ( req.startsWith("/a?ssid=") ) {
            for (int i = 0; i < 96; ++i) {
              EEPROM.write(i, 0);
            }
            String qsid; 
            qsid = req.substring(8,req.indexOf('&'));
            String qpass;
            qpass = req.substring(req.lastIndexOf('=')+1);

            for (int i = 0; i < qsid.length(); ++i) {
              EEPROM.write(i, qsid[i]);
            }

            for (int i = 0; i < qpass.length(); ++i) {
                EEPROM.write(32+i, qpass[i]);
            }    
            EEPROM.commit();
            s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Piano ";
            s += "Found ";
            s += req;
            s += "<p> saved to eeprom... reset to boot into new wifi</html>\r\n\r\n";
          }
          else
          {
            s = "HTTP/1.1 404 Not Found\r\n\r\n";
          }
          client.print(s);     
        }
      }
    }
  }
  
  Serial.println("All LEDs Coloured");
  // show colours
  for (int key = 0; key < keysCount; key++) {
    int offset = constrain(key%12,0,12);
    for (int i = 0; i < 2; i++)
      strip.SetPixelColor(i + (2*key), RgbColor(128*keyrColor[offset],128*keygColor[offset],128*keybColor[offset]));
  }
  strip.Show();

  my_mdns.Clear();
  struct mdns::Query query_applemidi;
  strncpy(query_applemidi.qname_buffer, QUESTION_SERVICE, MAX_MDNS_NAME_LEN);
  query_applemidi.qtype = MDNS_TYPE_PTR;
  query_applemidi.qclass = 1;    // "INternet"
  query_applemidi.unicast_response = 0;
  my_mdns.AddQuery(query_applemidi);
  my_mdns.Send();

  Serial.println("setting up midi network");
  
  AppleMIDI.begin("piano");
    
  AppleMIDI.OnConnected(OnAppleMidiConnected);
  AppleMIDI.OnDisconnected(OnAppleMidiDisconnected);

  AppleMIDI.OnReceiveNoteOn(OnAppleMidiNoteOn);
  AppleMIDI.OnReceiveNoteOff(OnAppleMidiNoteOff);

}

void loop() {
  // put your main code here, to run repeatedly:
 
  AppleMIDI.run();
  my_mdns.Check();
}


