#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contiki.h"
#include "contiki-net.h"

#define REST_RES_HELLO 1
#define REST_RES_CHUNKS 0
#define REST_RES_SEPARATE 0
#define REST_RES_PUSHING 1
#define REST_RES_EVENT 0
#define REST_RES_SUB 0
#define REST_RES_LEDS 1
#define REST_RES_TOGGLE 1

#if defined (PLATFORM_HAS_LIGHT) || IOTLAB_M3
#define REST_RES_LIGHT 1
#define REST_RES_TEMP 1
#endif

#define REST_RES_BATTERY 0
#define REST_RES_RADIO 0
#define REST_RES_MIRROR 0

#include "erbium.h"

#if defined IOTLAB_M3

#include <time.h>
#include "iotlab_i2c.h"
#include "iotlab_i2c_.h"
#include "i2c.h"
#include <platform.h>
#include "soft_timer_delay.h"
#include "phy.h"
#include "soft_timer.h"
#include "event.h"
#include "lps331ap.h"
#include "isl29020.h"
#include "iotlab_uid.h"
#include "mac_csma.h"
#include "phy.h"

#endif

#if defined WSN430

#include "clock.h"
#include "leds.h"
#include "uart0.h"
#include "tsl2550.h"
#include "ds1722.h"
#include "ds2411.h"
#include "timerA.h"

#endif

#include "iotlab_uid_num_hashtable.h"


#if defined (PLATFORM_HAS_BUTTON)
#include "dev/button-sensor.h"
#endif
#if defined (PLATFORM_HAS_LEDS)
#include "dev/leds.h"
#endif
#if defined (PLATFORM_HAS_LIGHT)
#include "dev/light-sensor.h"
#endif
#if defined (PLATFORM_HAS_BATTERY)
#include "dev/battery-sensor.h"
#endif
#if defined (PLATFORM_HAS_SHT11)
#include "dev/sht11-sensor.h"
#endif
#if defined (PLATFORM_HAS_RADIO)
#include "dev/radio-sensor.h"
#endif

#if WITH_COAP == 3
#include "er-coap-03.h"
#elif WITH_COAP == 7
#include "er-coap-07.h"
#elif WITH_COAP == 12
#include "er-coap-12.h"
#elif WITH_COAP == 13
#include "er-coap-13.h"
#else
#warning "Erbium example without CoAP-specifc functionality"
#endif

#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]",(lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3],(lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

/******************************************************************************/
/* VARIAVEIS GLOBAIS */
static int id = 1;
static const char *idRequisicao = NULL;
static const char *idSensor = NULL;

#if IOTLAB_M3
static struct {
    i2c_t i2c;
    uint8_t addr;
} iotlab_i2c;
#endif

/******************************************************************************/

#if IOTLAB_M3
int iotlab_get_time(struct soft_timer_timeval *time)
{
    uint8_t cmd = IOTLAB_I2C_RX_TIME;
    return i2c_tx_rx(iotlab_i2c.i2c, iotlab_i2c.addr, &cmd, 1,
            (uint8_t *)time, 8);
}
#endif

#if REST_RES_TEMP && (defined (PLATFORM_HAS_SHT11) || defined (IOTLAB_M3))
static float get_temp(void)
{
/*

Conforme a documentação encontrada para os dois casos, as medidas estão sendo retornadas em Graus Celsius

*/
#if defined PLATFORM_HAS_SHT11
  return ((sht11_sensor.value(SHT11_SENSOR_TEMP) / 10) - 396) / 10;
#endif
#if defined IOTLAB_M3
  int16_t value;
  lps331ap_read_temp(&value);
  return 42.5 + value / 480.0;
#endif
}

/* Leitura do sensor de temperatura */

PERIODIC_RESOURCE(temperature, METHOD_GET, "temperature", "title=\"Temperature (supports JSON)\";rt=\"TempSensor\"", CLOCK_SECOND);

void
temperature_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
#if IOTLAB_M3
    // Query control node time
  struct soft_timer_timeval time;
  if (iotlab_get_time(&time)) {
      printf("Error while getting Control node time\n");
      return;
  }
  
  struct tm *local_time = gmtime((time_t *)&time.tv_sec);
  char date_str[22];
  strftime(date_str, (sizeof date_str), "%Y-%m-%dT%H:%M:%SZ", local_time);
#endif
  uint16_t temp = get_temp();

  REST.get_query_variable(request, "idMsg", &idRequisicao);
  REST.get_query_variable(request, "idSensor", &idSensor);
  REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
#if IOTLAB_M3
  snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "{'d':%.1f,'idS':%u,'iReq':%u,'iRes':%d,'t':'%s'}", (float)temp,atoi(idSensor),atoi(idRequisicao), id, date_str);
#endif
#if !IOTLAB_M3
snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "{'d':%.1f,'idS':%u,'iReq':%u,'iRes':%d,'t':'-1'}", (float)temp,atoi(idSensor),atoi(idRequisicao), id);
#endif

  REST.set_response_payload(response, buffer, strlen((char *)buffer));
  id = (id + 1) % 10000;
}

void
temperature_periodic_handler(resource_t *r)
{
}

#endif /* PLATFORM_HAS_SHT11 */
/******************************************************************************/
#if REST_RES_LIGHT && (defined (PLATFORM_HAS_LIGHT) || defined (IOTLAB_M3))
static float get_light(void)
{

/*
Conforme as documentações disponíveis, essas medidas estão sendo feitas em lux
*/
#if defined IOTLAB_M3
  return isl29020_read_sample();
#endif
#if defined PLATFORM_HAS_LIGHT
  int light_val = light_sensor.value(0);
  return ((float)light_val) / LIGHT_SENSOR_VALUE_SCALE;
#endif
}

PERIODIC_RESOURCE(light, METHOD_GET, "light", "title=\"Photosynthetic and solar light (supports JSON)\";rt=\"LightSensor\"", CLOCK_SECOND);

void
light_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
#if IOTLAB_M3
    // Query control node time
  struct soft_timer_timeval time;
  if (iotlab_get_time(&time)) {
      printf("Error while getting Control node time\n");
      return;
  }
  struct tm *local_time = gmtime((time_t *)&time.tv_sec);
  char date_str[22];
  strftime(date_str, (sizeof date_str), "%Y-%m-%dT%H:%M:%SZ", local_time);
#endif

  float light = get_light();

  REST.get_query_variable(request, "idMsg", &idRequisicao);
  REST.get_query_variable(request, "idSensor", &idSensor);
  REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
#if IOTLAB_M3
  snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "{'d':%.1f,'idS':%u,'iReq':%u,'iRes':%d,'t':'%s'}", light,atoi(idSensor),atoi(idRequisicao), id, date_str);
#endif
#if !IOTLAB_M3
snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "{'d':%.1f,'idS':%u,'iReq':%u,'iRes':%d,'t':'-1'}", light,atoi(idSensor),atoi(idRequisicao), id);
#endif

  REST.set_response_payload(response, buffer, strlen((char *)buffer));
  id = (id + 1) % 10000;
}

void
light_periodic_handler(resource_t *r)
{
}
#endif /* PLATFORM_HAS_LIGHT */
/******************************************************************************/

PROCESS(rest_server_example, "Erbium Example Server");
AUTOSTART_PROCESSES(&rest_server_example);

PROCESS_THREAD(rest_server_example, ev, data)
{
  PROCESS_BEGIN();

  PRINTF("Starting Erbium Example Server\n");

#ifdef RF_CHANNEL
  PRINTF("RF channel: %u\n", RF_CHANNEL);
#endif
#ifdef IEEE802154_PANID
  PRINTF("PAN ID: 0x%04X\n", IEEE802154_PANID);
#endif

  PRINTF("uIP buffer: %u\n", UIP_BUFSIZE);
  PRINTF("LL header: %u\n", UIP_LLH_LEN);
  PRINTF("IP+UDP header: %u\n", UIP_IPUDPH_LEN);
  PRINTF("REST max chunk: %u\n", REST_MAX_CHUNK_SIZE);

  /* Inicializa a engine REST. */
  rest_init_engine();
#if IOTLAB_M3
  /*Inicializando o timer*/
  iotlab_i2c.i2c  = i2c1;
  iotlab_i2c.addr = IOTLAB_I2C_CN_ADDR;
#endif
  /* Ativa os recursos disponíveis. */

#if IOTLAB_M3 && REST_RES_LIGHT
  // ISL29020 light sensor initialisation
  isl29020_prepare(ISL29020_LIGHT__AMBIENT, ISL29020_RESOLUTION__16bit,
            ISL29020_RANGE__16000lux);
  isl29020_sample_continuous();
  rest_activate_periodic_resource(&periodic_resource_light);
#endif

#if IOTLAB_M3 && REST_RES_TEMP
  // LPS331AP pressure sensor initialisation
  lps331ap_powerdown();
  lps331ap_set_datarate(LPS331AP_P_12_5HZ_T_12_5HZ);
  rest_activate_periodic_resource(&periodic_resource_temperature);
#endif

#if (PLATFORM_HAS_LIGHT) && REST_RES_LIGHT
  light_sensor.configure(LIGHT_SENSOR_SOURCE, ISL29020_LIGHT__AMBIENT);
  light_sensor.configure(LIGHT_SENSOR_RESOLUTION, ISL29020_RESOLUTION__16bit);
  light_sensor.configure(LIGHT_SENSOR_RANGE, ISL29020_RANGE__1000lux);
  SENSORS_ACTIVATE(light_sensor);
  rest_activate_periodic_resource(&periodic_resource_light);
#endif

#if (PLATFORM_HAS_SHT11) && REST_RES_TEMP
  SENSORS_ACTIVATE(sht11_sensor);
  rest_activate_periodic_resource(&periodic_resource_temperature);
#endif

  /* eventos específicos da aplicação. */
  while(1) {
    PROCESS_WAIT_EVENT();
#if defined (PLATFORM_HAS_BUTTON)
    if (ev == sensors_event && data == &button_sensor) {
      PRINTF("BUTTON\n");
    }
#endif /* PLATFORM_HAS_BUTTON */
  }

  PROCESS_END();
}
