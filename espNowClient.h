#ifndef IEsk8Device
#include <IEsk8Device.h>
#endif
#include <esp_now.h>
#include <WiFi.h>

#define CHANNEL 1

void initESPNow();

esp_now_peer_info_t peer;
bool printStatus(esp_err_t status);

class EspNowClient : public IEsk8Device
{
public:
  EspNowClient()
  {
  }

  void connect()
  {
  }

  void update() {}

  void sendPacket(uint8_t *data, int data_len)
  {
    const uint8_t *peer_addr = peer.peer_addr;
    esp_err_t result = esp_now_send(peer_addr, data, data_len);
  }

  void setOnConnectedEvent(callBack ptr_onConnectedEvent)
  {
    _onConnectedEvent = ptr_onConnectedEvent;
  }
  void setOnDisconnectedEvent(callBack ptr_onDisconnectedEvent)
  {
    _onDisconnectedEvent = ptr_onDisconnectedEvent;
  }
  void setOnNotifyEvent(notifyCallBack ptr_onNotifyEvent)
  {
    _onNotifyEvent = ptr_onNotifyEvent;
  }
  void setOnSentEvent(callBack ptr_onSentEvent)
  {
    _onSentEvent = ptr_onSentEvent;
  }

private:
};

#ifndef espnowClient
EspNowClient client;
#endif

void configDeviceAP();

void addPeer(const uint8_t *mac_addr)
{
  Serial.printf("Peer does not exist!\n");
  peer.channel = CHANNEL;
  peer.encrypt = 0;
  peer.ifidx = ESP_IF_WIFI_AP;
  int status = esp_now_add_peer(&peer);
  switch (status)
  {
  case 0:
    Serial.printf("Added!\n");
    break;
  default:
    Serial.printf("Added ERROR!\n");
  }
}

#define DELETEBEFOREPAIR 0

bool printStatus(esp_err_t status)
{
  if (status == ESP_OK)
  {
    // Pair success
    Serial.println("Success");
    return true;
  }
  else if (status == ESP_ERR_ESPNOW_NOT_INIT)
  {
    // How did we get so far!!
    Serial.println("ESPNOW Not Init");
    return false;
  }
  else if (status == ESP_ERR_ESPNOW_ARG)
  {
    Serial.println("Invalid Argument");
    return false;
  }
  else if (status == ESP_ERR_ESPNOW_FULL)
  {
    Serial.println("Peer list full");
    return false;
  }
  else if (status == ESP_ERR_ESPNOW_NO_MEM)
  {
    Serial.println("Out of memory");
    return false;
  }
  else if (status == ESP_ERR_ESPNOW_EXIST)
  {
    Serial.println("Exists");
    return true;
  }
  else
  {
    Serial.println("Not sure what happened");
    return false;
  }
}

void deletePeer()
{
  esp_err_t delStatus = esp_now_del_peer(peer.peer_addr);
  Serial.print("Slave Delete Status: ");
  printStatus(delStatus);
}

// Check if the slave is already paired with the master.
// If not, pair the slave with master
bool pairPeer()
{
  if (peer.channel == CHANNEL)
  {
    if (DELETEBEFOREPAIR)
    {
      deletePeer();
    }

    Serial.print("Peer Status: ");
    // check if the peer exists
    bool exists = esp_now_is_peer_exist(peer.peer_addr);
    if (exists)
    {
      // Slave already paired.
      Serial.println("Already Paired");
      return true;
    }
    else
    {
      // Slave not paired, attempt pair
      esp_err_t addStatus = esp_now_add_peer(&peer);
      return printStatus(addStatus);
    }
  }
  else
  {
    // No slave found to process
    Serial.println("No Peer found to process");
    return false;
  }
}

// Scan for peers in AP mode
void ScanForPeer()
{
  int8_t scanResults = WiFi.scanNetworks();
  // reset on each scan
  bool peerFound = 0;
  memset(&peer, 0, sizeof(peer));

  Serial.println("");
  if (scanResults == 0)
  {
    Serial.println("No WiFi devices in AP Mode found");
  }
  else
  {
    Serial.printf("Found %d devices\n", scanResults);
    for (int i = 0; i < scanResults; ++i)
    {
      // Print SSID and RSSI for each device found
      String SSID = WiFi.SSID(i);
      int32_t RSSI = WiFi.RSSI(i);
      String BSSIDstr = WiFi.BSSIDstr(i);

      delay(10);
      // Check if the current device starts with `peer`
      if (SSID.indexOf("Slave") == 0)
      {
        // SSID of interest
        Serial.printf("Found a peer.%d: %s (RSSI: %d)\n", i + 1, SSID, RSSI);
        // Get BSSID => Mac Address of the peer
        int mac[6];
        if (6 == sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]))
        {
          for (int ii = 0; ii < 6; ++ii)
          {
            peer.peer_addr[ii] = (uint8_t)mac[ii];
          }
        }

        peer.channel = CHANNEL; // pick a channel
        peer.encrypt = 0;       // no encryption

        peerFound = 1;
        // we are planning to have only one peer in this example;
        // Hence, break after we find one, to be a bit efficient
        break;
      }
    }
  }

  if (peerFound)
  {
    Serial.println("Peer Found, processing..");
  }
  else
  {
    Serial.println("Peer Not Found, trying again.");
  }

  // clean up ram
  WiFi.scanDelete();
}

void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
  for (int i = 0; i < 6; i++)
  {
    peer.peer_addr[i] = (uint8_t)mac_addr[i];
  }

  if (!esp_now_is_peer_exist(mac_addr))
  {
    addPeer(mac_addr);
  }

  client._onNotifyEvent(data, data_len);
}

// callback when data is sent from Master to peer
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  client._onSentEvent();
}

// Init ESP Now with fallback
void initESPNow(bool master)
{
  if (master)
  {
    WiFi.disconnect();
    if (esp_now_init() == ESP_OK)
    {
      Serial.println("ESPNow Init Success");
    }
    else
    {
      Serial.println("ESPNow Init Failed");
      // Retry InitESPNow, add a counte and then restart?
      // InitESPNow();
      // or Simply Restart
      //ESP.restart();
    }
  }
  else
  {
    WiFi.mode(WIFI_AP);
    configDeviceAP();
    WiFi.disconnect();
    if (esp_now_init() == ESP_OK)
    {
      client._onConnectedEvent();
    }
    else
    {
      client._onDisconnectedEvent();
    }
  }
  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSent);
}

// config AP SSID
void configDeviceAP()
{
  const char *SSID = "Slave_1";
  bool result = WiFi.softAP(SSID, "peer_1_Password", CHANNEL, 0);
  if (!result)
  {
    Serial.println("AP Config failed.");
  }
  else
  {
    Serial.println("AP Config Success. Broadcasting with AP: " + String(SSID));
  }
}
