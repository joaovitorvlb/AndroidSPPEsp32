/*
  Espressif SDK-IDF: v3.2
Options choose step:
    1. make menuconfig.
    2. enter menuconfig "Component config", choose "Bluetooth"
    3. enter menu Bluetooth, choose "Classic Bluetooth" and "SPP Profile"
    4. choose your options.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/uart.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"

#include "driver/gpio.h"


#include "time.h"
#include "sys/time.h"

#define SPP_TAG "BLUE"
#define SPP_SERVER_NAME "SPP_SERVER"
#define DEVICE_NAME "BLUETOOTH SPP"

#define TESTE "{TESTE %d}"

uint8_t num = 0;

char buff[32];

#define DEBUG 1
#define BUTTON	GPIO_NUM_17 

//----------------------------------------
#define TXD  (GPIO_NUM_4)
#define RXD  (GPIO_NUM_16)
#define RTS  (UART_PIN_NO_CHANGE)
#define CTS  (UART_PIN_NO_CHANGE)

#define BUF_SIZE (512)
QueueHandle_t Queue_Uart;
//----------------------------------------

static const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;
static const esp_spp_sec_t sec_mask = ESP_SPP_SEC_AUTHENTICATE;
static const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;

QueueHandle_t Queue_Button;
static EventGroupHandle_t ble_event_group;
const static int CONNECTED_BIT = BIT0;
int socket_blue;


struct AMessage
 {
    int ucMessageID;
    char ucData[ 512 ];
 } xMessage;
struct AMessage pxMessage;

static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    switch (event) {
    case ESP_SPP_INIT_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_INIT_EVT");
        esp_bt_dev_set_device_name(DEVICE_NAME);
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        esp_spp_start_srv(sec_mask,role_slave, 0, SPP_SERVER_NAME);
        break;
    case ESP_SPP_DISCOVERY_COMP_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_DISCOVERY_COMP_EVT");
        break;
    case ESP_SPP_OPEN_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_OPEN_EVT");
        break;
    case ESP_SPP_CLOSE_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_CLOSE_EVT");
		xEventGroupClearBits(ble_event_group, CONNECTED_BIT);
        break;
    case ESP_SPP_START_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_START_EVT");
        break;
    case ESP_SPP_CL_INIT_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_CL_INIT_EVT");
        break;
    case ESP_SPP_DATA_IND_EVT:

        ESP_LOGI(SPP_TAG, "ESP_SPP_DATA_IND_EVT len=%d handle=%d", param->data_ind.len, param->data_ind.handle);
        
        sprintf(buff, TESTE, num);
        
        num++;

        esp_spp_write(socket_blue, (int)strlen(buff), (uint8_t*)buff);
	    
		pxMessage.ucMessageID  = param->data_ind.len;
		sprintf(pxMessage.ucData,"%.*s",param->data_ind.len, param->data_ind.data);
		//xQueueSend(Queue_Uart,(void *)&pxMessage,(TickType_t )0); 
        break;
    case ESP_SPP_CONG_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_CONG_EVT");
        break;
    case ESP_SPP_WRITE_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_WRITE_EVT");
        break;
    case ESP_SPP_SRV_OPEN_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_SRV_OPEN_EVT");
		
		xEventGroupSetBits(ble_event_group, CONNECTED_BIT);
		socket_blue = param->srv_open.handle; 
        break;
    default:
        break;
    }
}

void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT:{
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(SPP_TAG, "authentication success: %s", param->auth_cmpl.device_name);
            esp_log_buffer_hex(SPP_TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
        } else {
            ESP_LOGE(SPP_TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
        }
        break;
    }
    case ESP_BT_GAP_PIN_REQ_EVT:{
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);
        if (param->pin_req.min_16_digit) {
            ESP_LOGI(SPP_TAG, "Input pin code: 0000 0000 0000 0000");
            esp_bt_pin_code_t pin_code = {0};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
        } else {
            ESP_LOGI(SPP_TAG, "Input pin code: 1234");
            esp_bt_pin_code_t pin_code;
            pin_code[0] = '1';
            pin_code[1] = '2';
            pin_code[2] = '3';
            pin_code[3] = '4';
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
        }
        break;
    }


    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d", param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d", param->key_notif.passkey);
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
        break;


    default: {
        ESP_LOGI(SPP_TAG, "event: %d", event);
        break;
    }
    }
    return;
}


void Task_Button ( void *pvParameter )
{
    char res[10]; 
    volatile int aux=0;
	uint32_t counter = 0;
		
    /* Configura a GPIO 17 como saída */ 
	gpio_pad_select_gpio( BUTTON );	
    gpio_set_direction( BUTTON, GPIO_MODE_INPUT );
	gpio_set_pull_mode( BUTTON, GPIO_PULLUP_ONLY ); 
	
    printf("Leitura da GPIO BUTTON\n");
    
    for(;;) 
	{
	
	    xEventGroupWaitBits(ble_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
		
		if(gpio_get_level( BUTTON ) == 0 && aux == 0)
		{ 
		  /* Delay para tratamento do bounce da tecla*/
		  vTaskDelay( 80/portTICK_PERIOD_MS );	
		  
		  if(gpio_get_level( BUTTON ) == 0) 
		  {		
			
			sprintf(res, "%d\r\n", counter);
			printf("\r\nsocket n: %d.\r\n", socket_blue);	
			if(esp_spp_write(socket_blue, (int)strlen(res), (uint8_t*)res) == ESP_OK)
			{
			    counter++;
			} else {
			    printf("\r\nFalha no envio.\r\n");
			}
			
			aux = 1; 
		  }
		}

		if(gpio_get_level( BUTTON ) == 1 && aux == 1)
		{
		    vTaskDelay( 80/portTICK_PERIOD_MS );	
			if(gpio_get_level( BUTTON ) == 1 )
			{
				aux = 0;
			}
		}	

		vTaskDelay( 10/portTICK_PERIOD_MS );	
    }
}


static void Task_Uart( void *pvParameter )
{
    struct AMessage pxMessage;
	
    /* Configuração do descritor da UART */
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
	
	/* Define qual a UART a ser configurada. Neste caso, a UART_NUM_1 */ 
	/* No ESP32 há três UART, sendo a UART_NUM_0 utilizada para o debug, enquanto a UART_NUM_1 e UART_NUM_2 
	   estão livres para uso geral. 
	*/
    uart_param_config(UART_NUM_1, &uart_config);
	
	/* Definimos quais os pinos que serão utilizados para controle da UART_NUN_1 */
    uart_set_pin(UART_NUM_1, TXD, RXD, RTS, CTS);
	
	/* Define o buffer de recepção (RX) com 1024 bytes, e mantem o buffer de transmissão em zero. Neste caso, 
	   a task permanecerá bloqueada até que o conteúdo (charactere) seja transmitido. O três últimos parametros
	   estão relacionados a interrupção (evento) da UART.*/
	   
    uart_driver_install(UART_NUM_1, BUF_SIZE * 1, 0, 0, NULL, 0);

    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    int len;
	
    for(;;) 
	{      
	   
	   /* Leitura da UART */
       len = uart_read_bytes(UART_NUM_1, data, BUF_SIZE, 20 / portTICK_RATE_MS);
	   if(len>0)
	   {
	        if(esp_spp_write(socket_blue, len, (uint8_t*)data) == ESP_OK)
			{
			   printf("\r\nMCU->MSG: %.*s\r\n", len, (uint8_t*)data );
			} else {
			    printf("\r\nFalha no envio.\r\n");
			}
	   }
	   
        if( Queue_Uart != NULL ) 
		{
		   if(xQueueReceive(Queue_Uart, &(pxMessage), 0))
		   {
		        printf("BLE->MSG: %.*s", pxMessage.ucMessageID,pxMessage.ucData);
				uart_write_bytes(UART_NUM_1, (char *) pxMessage.ucData, pxMessage.ucMessageID);
		   }
        }		         
		
		vTaskDelay(500/portTICK_PERIOD_MS); 
    }
}


void app_main( void )
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );


	/*
	   Event Group do FreeRTOS. 
	   Só podemos enviar ou ler alguma informação quando a rede BLUE estiver configurada e pareada com o master BLUE.
	*/
	ble_event_group = xEventGroupCreate();
	
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    /* Inicializa o controlador do Bluetooth */
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

	/* Habilita e Verifica se o controlador do Bluetooth foi inicializado corretamente */
    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

	/* Bluedroid é o nome do Stack BLUE da Espressif. Inicializa o Stack Bluetooth*/
    if ((ret = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s initialize bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

	/* Bluedroid é o nome do Stack BLUE da Espressif. Habilita o Stack Bluetooth*/
    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s enable bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

	/* Habilita e verifica se o stack bluetooth foi inicializado corretamente */
    if ((ret = esp_bt_gap_register_callback(esp_bt_gap_cb)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s gap register failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

	/* Registra a função de Callback responsável em receber e tratar todos os eventos do rádio Bluetooth */
    if ((ret = esp_spp_register_callback(esp_spp_cb)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s spp register failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

	/* O bluetooth possui vários perfis. Dentre eles o SPP (Serial Port Profile). Este perfil pertence ao Bluetooth clássico
	e é por meio dele que fazemos uma comunicação serial transparente (ponto a ponto). 
	Habilita e verifica se o perfil SPP do Bluetooth.*/
    if ((ret = esp_spp_init(esp_spp_mode)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s spp init failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    /* Set default parameters for Secure Simple Pairing */
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    //ESP_BT_IO_CAP_NONE;
	//ESP_BT_IO_CAP_OUT;
	//ESP_BT_IO_CAP_IO;
	//ESP_BT_IO_CAP_IN;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));

    /*
     * Set default parameters for Legacy Pairing
     * Use variable pin, input pin code when pairing
     */
    esp_bt_pin_type_t pin_type =  ESP_BT_PIN_TYPE_VARIABLE;//ESP_BT_PIN_TYPE_FIXED;//ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;  
    esp_bt_gap_set_pin(pin_type, 0, pin_code);
	
	
	/*
	   Task responsável em ler e enviar valores via Socket TCP Client. 
	*/
	if( xTaskCreate( Task_Button, "TaskButton", 10000, NULL, 1, NULL ) != pdTRUE )
    {
      if( DEBUG )
        ESP_LOGI( SPP_TAG, "error - Nao foi possivel alocar Task_Button.\r\n" );  
      return;   
    }
	
	if( xTaskCreate( Task_Uart, "TaskUart", 10000, NULL, 3, NULL ) != pdTRUE )
    {
      if( DEBUG )
        ESP_LOGI( SPP_TAG, "error - Nao foi possivel alocar Task_Uart.\r\n" );  
      return;   
    }
    
}
