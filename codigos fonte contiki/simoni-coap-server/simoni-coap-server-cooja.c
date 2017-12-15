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
#define REST_RES_LIGHT 1
#define REST_RES_TEMP 0
#define REST_RES_BATTERY 0
#define REST_RES_RADIO 0
#define REST_RES_MIRROR 0

#include "erbium.h"

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
static char *idRequisicao = NULL;
static char *idSensor = NULL;

/******************************************************************************/

#if REST_RES_TEMP && defined (PLATFORM_HAS_SHT11)

static uint16_t
get_temp(void)
{
  //GRAUS CELSIUS
  return ((sht11_sensor.value(SHT11_SENSOR_TEMP) / 10) - 396) / 10;
}

/* Leitura do sensor de temperatura */

PERIODIC_RESOURCE(temperature, METHOD_GET, "temperature", "title=\"Temperature (supports JSON)\";rt=\"TempSensor\"", CLOCK_SECOND);

void
temperature_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  uint16_t temp = get_temp();

  REST.get_query_variable(request, "idMsg", &idRequisicao);
  REST.get_query_variable(request, "idSensor", &idSensor);
  REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
  snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "{'d':%u, 'idS':%u,'iReq':%u, 'iRes': %d,'t': %d}", temp,atoi(idSensor),atoi(idRequisicao), id,clock_seconds());

  REST.set_response_payload(response, buffer, strlen((char *)buffer));
  id = (id + 1) % 10000;
}

void
temperature_periodic_handler(resource_t *r)
{
}

#endif /* PLATFORM_HAS_SHT11 */
/******************************************************************************/
#if REST_RES_LIGHT && defined (PLATFORM_HAS_LIGHT)
static uint16_t
get_light(void)
{
  return 10 * light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC) / 7;
}

PERIODIC_RESOURCE(light, METHOD_GET, "light", "title=\"Photosynthetic and solar light (supports JSON)\";rt=\"LightSensor\"", CLOCK_SECOND);

void
light_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  uint16_t light = get_light();

  REST.get_query_variable(request, "idMsg", &idRequisicao);
  REST.get_query_variable(request, "idSensor", &idSensor);
  REST.set_header_content_type(response, REST.type.APPLICATION_JSON);
  snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "{'d':%u, 'idS':%u,'iReq':%u, 'iRes': %d,'t': %d}", light,atoi(idSensor),atoi(idRequisicao), id,1);

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

  clock_init();

  /* Ativa os recursos disponíveis. */

#if defined (PLATFORM_HAS_LIGHT) && REST_RES_LIGHT
  SENSORS_ACTIVATE(light_sensor);
  rest_activate_periodic_resource(&periodic_resource_light);
#endif
#if defined (PLATFORM_HAS_SHT11) && REST_RES_TEMP
  SENSORS_ACTIVATE(sht11_sensor);
//  rest_activate_resource(&resource_temperature);
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
