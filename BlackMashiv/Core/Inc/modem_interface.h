/*
 * modem_interface.h
 *
 *  Created on: Dec 2, 2024
 *      Author: sadab
 */

#ifndef INC_MODEM_INTERFACE_H_
#define INC_MODEM_INTERFACE_H_

#define ONE_S_DELAY 1000
#define DELAY_300_MS 300
#define DELAY_5_S (5 * ONE_S_DELAY)
#define DELAY_1_5_S (1.5 * ONE_S_DELAY)
#define DELAY_1_8_S (1.8 * ONE_S_DELAY)
#define DELAY_150_S (150 * ONE_S_DELAY)
#define PDP_SETTING_CMD_STATIC_LENGTH 20
#define MAX_APN_LENGTH 5

#define SIM_READY 0
#define SIM_NOT_READY -1

#include "main.h"
#include <string.h>

extern UART_HandleTypeDef huart1;

extern uint8_t usart1_rx_buffer[USART1_RX_BUFFER_SIZE];
extern uint8_t usart1_tx_buffer[USART1_TX_BUFFER_SIZE];

extern uint8_t* usb_tx_buffer;
extern uint8_t* usb_rx_buffer;

// Structure declaration for an AT command with arguments
typedef struct {
    char* execution_cmd;  // Base command (e.g., "AT+CMD")
    int delay;       // Delay after the command (in milliseconds)
    char* response;       // Response buffer or expected response
} AT_CMD;

typedef struct {
    uint8_t sim_status;   // 0 if SIM is ready, -1 otherwise
    char* sim_message; // Message describing the SIM status
} SIMStatus;

typedef struct{
	uint8_t net_registration_stat_numeric;
	char* net_registration_stat_verbose;
}NetRegistrationStat;

typedef struct{
	uint8_t cid;
	char* IP_type;
	char* apn_name;
	char* PDP_address;
	char* define_PDP_cmd;
}PDPSettings;

typedef enum
{
	STAT_NOT_REGISTERED = 0,
	STAT_REGISTERED_HOME = 1,
	STAT_NOT_REGISTERED_SEARCHING = 2,
	STAT_REGISTRATION_DENIED = 3,
	STAT_UNKNOWN = 4,
	STAT_REGISTERED_ROAMING = 5
} NET_STAT;

typedef enum
{
	GENERAL_ERR_CODE = -1,
	OK_CODE = 0,
	GENERAL_MODEM_ERROR = 1,
	SIM_ERROR = 2,
	NETWORK_ERROR = 3
} ERR_CODE;

typedef enum {
    NOT_REGISTERED = 0, // Not registered. MT is not currently searching an operator to register to.
    REGISTERED_HOME_NETWORK = 1, // Registered, home network
    SEARCHING_OPERATOR = 2, // Not registered, but MT is currently trying to attach or searching an operator to register to.
    REGISTRATION_DENIED = 3, // Registration denied
    UNKNOWN = 4, // Unknown
    REGISTERED_ROAMING = 5 // Registered, roaming
} NetworkRegistrationStatus;

typedef enum {
	UNKNOWN_RAT = -1,
	GSM = 0,
	EMTC = 8,
	NBIOT = 9,
}RAT;


//char temp_buf[300];
//char temp_buf2[300];
//
//AT_CMD get_imei = {"AT+GSN\r\n", DELAY_300_MS, temp_buf };
//AT_CMD get_modem_version = { "AT+GMR\r\n", DELAY_300_MS, temp_buf2 };

uint8_t send_AT_cmd(AT_CMD cmd);

char* get_modem_data(char* data);
char* get_sim_data(char* data);
char* get_pdp_data(uint8_t pdp_id, char* data);
char* query_net_status(char* stat);
char* query_net_data(char* data);
char* set_apn_func(char* data);
void modem_info_parser(const char* response, char* imei, size_t imei_size, char* fw_version, size_t fw_size);
void sim_info_parser(const char* response, char* iccid, size_t iccid_size, char* imsi, size_t imsi_size);
uint8_t checkSimStatus(const char *response, SIMStatus *status);
char* force_lte_scan(char* data);
uint8_t getNetworkRegistrationStatusDescription(const char* response, NetRegistrationStat* net_stat);
int get_operator_parse_response(const char* response, char* output_message);
int parse_open_socket_response(const char* response, char* output_msg);
char* activate_pdp_context(char* data);
char* deactivate_pdp_context(char* data);
char* check_operator_selection(char* data);
char* open_socket(char* data);

void clear_uart_rx_buffer(void);
void build_pdp_command(PDPSettings* pdp_struct);
uint8_t send_data_to_usb(char* data);
uint8_t recieve_data_from_usb(char* recieve_buffer);



#endif /* INC_MODEM_INTERFACE_H_ */


