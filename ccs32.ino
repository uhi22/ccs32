/* HomePlug Ethernet communication with WT32-ETH01 */
/* This is the main Arduino file of the project. */


/* some snippets from ETH.h */
#include "esp_system.h"
#include "esp_eth.h"
#define ETH_PHY_ADDR 1 /* from pins_arduino.h of wt32-eth01 */
#define ETH_PHY_TYPE ETH_PHY_LAN8720

#ifndef ETH_PHY_MDC
#define ETH_PHY_MDC 23
#endif

#ifndef ETH_PHY_MDIO
#define ETH_PHY_MDIO 18
#endif

#ifndef ETH_CLK_MODE
#define ETH_CLK_MODE ETH_CLOCK_GPIO0_IN
#endif

typedef enum { ETH_CLOCK_GPIO0_IN, ETH_CLOCK_GPIO0_OUT, ETH_CLOCK_GPIO16_OUT, ETH_CLOCK_GPIO17_OUT } eth_clock_mode_t;

/* some snippets from ETH.cpp */
#include "esp_event.h"
#include "esp_eth.h"
#include "esp_eth_phy.h"
#include "esp_eth_mac.h"
#include "esp_eth_com.h"
#include "soc/emac_ext_struct.h"
#include "soc/rtc.h"
#include "lwip/err.h"
#include "lwip/dns.h" 

static eth_clock_mode_t eth_clock_mode = ETH_CLK_MODE;
esp_eth_handle_t eth_handle;

/* The logging macros */
#undef log_v
#undef log_e
#define log_v(format, ...) log_printf(ARDUHAL_LOG_FORMAT(V, format), ##__VA_ARGS__)
#define log_e(format, ...) log_printf(ARDUHAL_LOG_FORMAT(E, format), ##__VA_ARGS__)


/*********************************************/

#define LED 2 /* The IO2 is used for an LED. This LED is externally added to the WT32-ETH01 board. */
uint32_t currentTime;
uint32_t lastTime1s;
uint32_t lastTime30ms;
uint32_t nCycles30ms;
uint8_t ledState;
uint32_t nTotalEthReceiveBytes; /* total number of bytes which has been received from the ethernet port */
#define MY_ETH_TRANSMIT_BUFFER_LEN 200
uint8_t mytransmitbuffer[MY_ETH_TRANSMIT_BUFFER_LEN];
uint8_t mytransmitbufferLen=0; /* The number of used bytes in the ethernet transmit buffer */
#define MY_ETH_RECEIVE_BUFFER_LEN 200
uint8_t myreceivebuffer[MY_ETH_RECEIVE_BUFFER_LEN];
uint8_t myMAC[6] = {0xDC, 0x0e, 0xa1, 0x11, 0x67, 0x09}; /* just a default MAC address. Will be overwritten by the PHY MAC. */

/* based on template in WiFiGeneric.cpp, function _arduino_event_cb() */
void myEthernetEventCallback(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
	if (event_base == ETH_EVENT && event_id == ETHERNET_EVENT_CONNECTED) {
		log_v("Ethernet Link Up");
	} else if (event_base == ETH_EVENT && event_id == ETHERNET_EVENT_DISCONNECTED) {
		log_v("Ethernet Link Down");
	} else if (event_base == ETH_EVENT && event_id == ETHERNET_EVENT_START) {
		log_v("Ethernet Started");
	} else if (event_base == ETH_EVENT && event_id == ETHERNET_EVENT_STOP) {
		log_v("Ethernet Stopped");
  }    
}

/* The receive function, which is called by the esp32-ethernet-driver. */
esp_err_t myEthernetReceiveCallback(esp_eth_handle_t hdl, uint8_t *buffer, uint32_t length, void *priv) {
  nTotalEthReceiveBytes+=length;
  /* We received an ethernet package. Determine its type, and dispatch it to the related handler. */
  uint16_t etherType = getEtherType(buffer);
  //Serial.println("EtherType" + String(etherType, HEX) + " size " + String(length));   
  if (etherType == 0x88E1) { /* it is a HomePlug message */
    //Serial.println("Its a HomePlug message.");    
    uint32_t L;
    L=length;
    if (L>=MY_ETH_RECEIVE_BUFFER_LEN) L=MY_ETH_RECEIVE_BUFFER_LEN;
    memcpy(myreceivebuffer, buffer, L); 
    evaluateReceivedHomeplugPacket();
  }
  if (etherType == 0x86dd) { /* it is an IPv6 frame */
      //self.ipv6.evaluateReceivedPacket(pkt)
  }
  return ESP_OK;       
}

/* The Ethernet transmit function. */
void myEthTransmit(void) {
  (void)esp_eth_transmit(eth_handle, mytransmitbuffer, mytransmitbufferLen);
}

/* The Ethernet initialization function.
   Based on code snippets from ETH.cpp ETHClass::begin */
bool initEth(void) {
  uint8_t i;
  int rc;
  uint8_t phy_addr=ETH_PHY_ADDR;
  int power=ETH_PHY_POWER;
  int mdc=ETH_PHY_MDC;
  int mdio=ETH_PHY_MDIO;

  //log_v("This is initEth.");  
	//log_printf(ARDUHAL_LOG_FORMAT(I, "phy_addr %d"), phy_addr);
	//log_printf(ARDUHAL_LOG_FORMAT(I, "power %d"), power);
	//log_printf(ARDUHAL_LOG_FORMAT(I, "mdc %d"), mdc);
	//log_printf(ARDUHAL_LOG_FORMAT(I, "mdio %d"), mdio);
	//log_printf(ARDUHAL_LOG_FORMAT(I, "type %d"), type);
	//log_printf(ARDUHAL_LOG_FORMAT(I, "clock_mode %d"), clock_mode);
  esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
  esp_netif_t *eth_netif = esp_netif_new(&cfg);
        
  esp_eth_mac_t *eth_mac = NULL;
  eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
  mac_config.clock_config.rmii.clock_mode = (eth_clock_mode) ? EMAC_CLK_OUT : EMAC_CLK_EXT_IN;
  mac_config.clock_config.rmii.clock_gpio = (1 == eth_clock_mode) ? EMAC_APPL_CLK_OUT_GPIO : (2 == eth_clock_mode) ? EMAC_CLK_OUT_GPIO : (3 == eth_clock_mode) ? EMAC_CLK_OUT_180_GPIO : EMAC_CLK_IN_GPIO;
  mac_config.smi_mdc_gpio_num = mdc;
  mac_config.smi_mdio_gpio_num = mdio;
  mac_config.sw_reset_timeout_ms = 1000;
    
  log_v("calling esp_eth_mac_new_esp32");  
  eth_mac = esp_eth_mac_new_esp32(&mac_config);
  if(eth_mac == NULL){
    log_e("esp_eth_mac_new_esp32 failed");
    return false;
  }
  //log_v("done");
    
  eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
  phy_config.phy_addr = phy_addr;
  phy_config.reset_gpio_num = power;
  esp_eth_phy_t *eth_phy = NULL;
    
  log_v("calling esp_eth_phy_new_lan8720");  
  eth_phy = esp_eth_phy_new_lan8720(&phy_config);
  if(eth_phy == NULL){
    log_e("esp_eth_phy_new failed");
    return false;
  }
  //log_v("done");

  eth_handle = NULL;
  esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(eth_mac, eth_phy);
  log_v("Calling esp_eth_driver_install.");    
  if(esp_eth_driver_install(&eth_config, &eth_handle) != ESP_OK || eth_handle == NULL){
    log_e("esp_eth_driver_install failed");
    return false;
  }
  //log_v("done");

  log_v("creating event loop");
  esp_err_t err = esp_event_loop_create_default();
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
  	log_e("esp_event_loop_create_default failed!");
    return false;
  }
  //log_v("done");

  /* registering the event callback from the ethernet driver */
  log_v("registering the event callback from the ethernet driver.");
  rc = esp_event_handler_instance_register(ETH_EVENT, ESP_EVENT_ANY_ID, &myEthernetEventCallback, NULL, NULL);
  //log_v("returned %d", rc);    
  if(rc) {
        log_e("event_handler_instance_register for ETH_EVENT Failed! rc %d", rc);
        return false;
  }
  //log_v("done");

  log_v("registering the receive callback from the ethernet driver.");
  rc = esp_eth_update_input_path(eth_handle, &myEthernetReceiveCallback, NULL);
  //log_v("returned %d", rc);    
  if(rc) {
      log_e("esp_eth_update_input_path Failed! rc %d", rc);
      return false;
  }
  //log_v("done");

  /* starting the ethernet driver in standalone mode, means without TCP/IP etc. */
  log_v("starting the ethernet driver in standalone mode.");    
  if(esp_eth_start(eth_handle) != ESP_OK){
    log_e("esp_eth_start failed");
    return false;
  }
  //log_v("esp_eth_start done");

  //log_v("requesting MAC."); 
  rc = esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, myMAC);
  log_v("myMAC %hx:%hx:%hx:%hx:%hx:%hx", myMAC[0], myMAC[1], myMAC[2], myMAC[3], myMAC[4], myMAC[5]); 
        
  // holds a few milliseconds to enter into a good state
  // FIX ME -- adresses issue https://github.com/espressif/arduino-esp32/issues/5733
  delay(50);
  return true; 
}

/**********************************************************/

void task30ms(void) {
	nCycles30ms++;
	runPevSequencer();
}

void task1s(void) {
  if (ledState==0) {
    digitalWrite(LED,HIGH);
    //Serial.println("LED on");
	ledState = 1;
  } else {
    digitalWrite(LED,LOW);
    //Serial.println("LED off");
	ledState = 0;
  }
  log_v("nTotalEthReceiveBytes=%ld, nCycles30ms=%ld", nTotalEthReceiveBytes, nCycles30ms);
  //sendTestFrame(); /* just for testing, send something on the Ethernet */
}

/**********************************************************/

void setup() {
  // Set pin mode
  pinMode(LED,OUTPUT);
  Serial.begin(115200);
  Serial.println("Started.");
  if (initEth()) {
    log_v("Ethernet initialized");
  } else {
    log_v("Error: Ethernet init failed.");
  }
  homeplugInit();
  log_v("Setup finished. The time for the tasks starts here.");
  currentTime = millis();  
	lastTime30ms = currentTime;
	lastTime1s = currentTime;
}

void loop() {
  currentTime = millis();
  if ((currentTime - lastTime30ms)>30) {
	  lastTime30ms += 30;
	  task30ms();
  }
  if ((currentTime - lastTime1s)>1000) {
	  lastTime1s += 1000;
	  task1s();
  }
}
