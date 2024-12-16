/*
 * modem_interface.c
 *
 *  Created on: Dec 2, 2024
 *      Author: sadab
 */

#include "modem_interface.h"
#include "usbd_cdc_if.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//extern AT_CMD get_imei, get_modem_version, check_sim_presence, force_lte, query_net_stat, set_apn;

const char* EOL = "\r\n";

const char* ati_cmd = "ATI\r\n";

const char* get_modem_version_cmd = "AT+GMR\r\n";
const char* get_Imei_cmd = "AT+GSN\r\n";

const char* get_Imsi_cmd = "AT+CIMI\r\n";
const char* get_Iccid_cmd = "AT+QCCID\r\n";

const char* get_apn_cmd = "AT+CGDCONT?\r\n";
const char* get_ip_addr_cmd = "AT+CGPADDR=";

const char* query_net_stat_cmd = "AT+CEREG?\r\n";
const char* check_sim_presence_cmd = "AT+CPIN?\r\n";
const char* check_operator_cmd = "AT+COPS?\r\n";
const char* force_lte_cmd = "AT+QCFG=\x22nwscanmode\x22,3\r\n";
const char* activate_pdp_context_cmd = "AT+CGACT=1,1\r\n";
const char* deactivate_pdp_context_cmd = "AT+CGACT=0,1\r\n";

char* sim_ready_msg = "SIM is ready.";
char* sim_state_msg = "SIM state:";
char* sim_invalid_state_msg = "Invalid response: No SIM state found.";

char* lte_network_status_intro = "LTE network registration status: ";

char* apn_name = "gh";
char* set_apn_cmd = "AT+CGDCONT=1,\x22IP\x22,\x22gh\x22\r\n";

char* open_socket_cmd = "AT+QIOPEN=1,1,\x22UDP\x22,\x22 8.1.14.252\x22,5000\r\n";
char* send_data_cmd = "at+qisend=1\r\n";
char send_data_terminator = '\x1A';

char* at_cmd_response;

AT_CMD get_imei, get_modem_version, check_sim_presence, force_lte, query_net_stat, set_apn, activate_pdp_context_AT, deactivate_pdp_context_AT, check_operator_AT, open_socket_AT, send_data_AT;


uint8_t send_AT_cmd(AT_CMD cmd)
{
	while(huart1.gState != HAL_UART_STATE_READY){}
 	HAL_UART_Transmit_IT(&huart1, (uint8_t*)(cmd.execution_cmd), strlen(cmd.execution_cmd));
 	while(huart1.RxState != HAL_UART_STATE_READY){}
	HAL_UARTEx_ReceiveToIdle_IT(&huart1, usart1_rx_buffer, USART1_RX_BUFFER_SIZE);
	return OK_CODE;
}

char* get_modem_data(char* data)
{
	get_imei.delay = DELAY_300_MS;
	get_imei.execution_cmd = malloc(strlen(get_Imei_cmd) + 1);
	strcpy(get_imei.execution_cmd, get_Imei_cmd);

	get_modem_version.delay = DELAY_300_MS;
	get_modem_version.execution_cmd = malloc(strlen(get_Imei_cmd) + 1);
	strcpy(get_modem_version.execution_cmd, get_modem_version_cmd);

	// Send AT commands and store responses
	send_AT_cmd(get_modem_version);
	free(get_modem_version.execution_cmd);

	get_modem_version.response = malloc(strlen((char*)usart1_rx_buffer) + 1);
	strcpy(get_modem_version.response, (char*)usart1_rx_buffer);
	clear_uart_rx_buffer();

	send_AT_cmd(get_imei);
	free(get_imei.response);

	get_imei.response = malloc(strlen((char*)usart1_rx_buffer) + 1);
	strcpy(get_imei.response, (char*)usart1_rx_buffer);
	clear_uart_rx_buffer();

	// Create a usb_rx_buffer for the final concatenated response
    char* concatenated = malloc(strlen(get_imei.response) + strlen(get_modem_version.response) + 1);


	// Concatenate responses into the final string
    strcpy(concatenated, get_imei.response);

    // Concatenate the second string
    strcat(concatenated, get_modem_version.response);

	// Copy the concatenated response to `data` (assuming `data` is defined elsewhere)
    usb_tx_buffer = malloc(strlen(concatenated) + 1);
	strcpy((char*)usb_tx_buffer, concatenated);
	free(concatenated);
	free(get_modem_version.response);
	free(get_imei.response);

    char imei[16] = {0};
    char fw_version[20] = {0};

    // Parse the response
    modem_info_parser((char*)usb_tx_buffer, imei, sizeof(imei), fw_version, sizeof(fw_version));
    snprintf((char*)usb_tx_buffer, 60, "Imei is: %s, fw version: %s\r\n", imei, fw_version);
	// transfer to user via usb
	CDC_Transmit_FS(usb_tx_buffer, strlen((char*)usb_tx_buffer));
	free(usb_tx_buffer);
	return data;
}

char* get_sim_data(char* data)
{
	// Create AT_CMD objects with appropriate buffers
	check_sim_presence.delay = DELAY_5_S;
	check_sim_presence.execution_cmd = malloc(strlen(check_sim_presence_cmd) + 1);
	strcpy(check_sim_presence.execution_cmd, check_sim_presence_cmd);


	// Send AT commands and store responses
	send_AT_cmd(check_sim_presence);
	check_sim_presence.response = malloc(strlen((char*)usart1_rx_buffer) + 1);
	strcpy(check_sim_presence.response, (char*)usart1_rx_buffer);
	clear_uart_rx_buffer();

	SIMStatus sim_stat;
	checkSimStatus(check_sim_presence.response, &sim_stat);
	if(sim_stat.sim_status == SIM_NOT_READY)
	{
		strcpy((char*)usb_tx_buffer, sim_stat.sim_message);
	}

	else{
		AT_CMD get_imsi;
		AT_CMD get_iccid;

		get_imsi.delay = DELAY_300_MS;
		get_imsi.execution_cmd = malloc(strlen(get_Imsi_cmd) + 1);
		strcpy(get_imsi.execution_cmd, get_Imsi_cmd);

		get_iccid.delay = DELAY_300_MS;
		get_iccid.execution_cmd = malloc(strlen(get_Iccid_cmd) + 1);
		strcpy(get_iccid.execution_cmd, get_Iccid_cmd);

		send_AT_cmd(get_imsi);
		free(get_imsi.execution_cmd);
		get_imsi.response = malloc(strlen((char*)usart1_rx_buffer) + 1);
		strcpy(get_imsi.response, (char*)usart1_rx_buffer);
		clear_uart_rx_buffer();

		send_AT_cmd(get_iccid);
		free(get_iccid.execution_cmd);
		get_iccid.response = malloc(strlen((char*)usart1_rx_buffer) + 1);
		strcpy(get_iccid.response, (char*)usart1_rx_buffer);
		clear_uart_rx_buffer();

		// Create a usb_rx_buffer for the final concatenated response
		char* concatenated = malloc(strlen(get_imsi.response) + strlen(get_iccid.response) + 1);


		// Concatenate responses into the final string
		strcpy(concatenated, get_iccid.response);

		// Concatenate the second string
		strcat(concatenated, get_imsi.response);

		// Copy the concatenated response to `data` (assuming `data` is defined elsewhere)

		free(get_iccid.response);
		free(get_imsi.response);

		char* imsi = malloc(16);
		char* iccid = malloc(23);

		sim_info_parser(concatenated, iccid, 22, imsi, 16);
		free(concatenated);
		// Parse the response
		usb_tx_buffer = malloc(140);
		snprintf((char*)usb_tx_buffer, 142, "Sim status: %s\r\nImsi is: %s, iccid: %s\r\n", sim_stat.sim_message, imsi, iccid);
		free(imsi);
		free(iccid);
	}
	free(sim_stat.sim_message);
	// transfer to user via usb
	CDC_Transmit_FS((uint8_t*)usb_tx_buffer, strlen((char*)usb_tx_buffer));
	free(usb_rx_buffer);
	return data;
}

char* force_lte_scan(char* data)
{
	force_lte.delay = DELAY_300_MS;
	force_lte.execution_cmd = malloc(strlen(force_lte_cmd) + 1);
	strcpy(force_lte.execution_cmd, force_lte_cmd);

	send_AT_cmd(force_lte);
	free(force_lte.execution_cmd);

	force_lte.response = malloc(sizeof(usart1_rx_buffer) + 1);
	strcpy(force_lte.response, (char*)usart1_rx_buffer);
	clear_uart_rx_buffer();


	usb_tx_buffer = malloc(strlen(force_lte.response));
	strcpy((char*)usb_tx_buffer, force_lte.response);
	free(force_lte.response);
	CDC_Transmit_FS(usb_tx_buffer, strlen((char*)usb_tx_buffer));
	free(usb_tx_buffer);
	return data;

}

char* activate_pdp_context(char* data)
{
	activate_pdp_context_AT.delay = DELAY_1_5_S;
	activate_pdp_context_AT.execution_cmd = malloc(strlen(activate_pdp_context_cmd) + 1);
	strcpy(activate_pdp_context_AT.execution_cmd, activate_pdp_context_cmd);

	send_AT_cmd(activate_pdp_context_AT);
	free(activate_pdp_context_AT.execution_cmd);

	activate_pdp_context_AT.response = malloc(sizeof(usart1_rx_buffer) + 1);
	strcpy(activate_pdp_context_AT.response, (char*)usart1_rx_buffer);
	clear_uart_rx_buffer();


	usb_tx_buffer = malloc(strlen(activate_pdp_context_AT.response));
	strcpy((char*)usb_tx_buffer, activate_pdp_context_AT.response);
	free(activate_pdp_context_AT.response);
	CDC_Transmit_FS(usb_tx_buffer, strlen((char*)usb_tx_buffer));
	free(usb_tx_buffer);
	return data;
}

char* deactivate_pdp_context(char* data)
{
	deactivate_pdp_context_AT.delay = DELAY_1_5_S;
	deactivate_pdp_context_AT.execution_cmd = malloc(strlen(deactivate_pdp_context_cmd) + 1);
	strcpy(deactivate_pdp_context_AT.execution_cmd, deactivate_pdp_context_cmd);

	send_AT_cmd(deactivate_pdp_context_AT);
	free(deactivate_pdp_context_AT.execution_cmd);

	deactivate_pdp_context_AT.response = malloc(sizeof(usart1_rx_buffer) + 1);
	strcpy(deactivate_pdp_context_AT.response, (char*)usart1_rx_buffer);
	clear_uart_rx_buffer();


	usb_tx_buffer = malloc(strlen(deactivate_pdp_context_AT.response));
	strcpy((char*)usb_tx_buffer, deactivate_pdp_context_AT.response);
	free(deactivate_pdp_context_AT.response);
	CDC_Transmit_FS(usb_tx_buffer, strlen((char*)usb_tx_buffer));
	free(usb_tx_buffer);
	return data;
}

char* check_operator_selection(char* data)
{
	check_operator_AT.delay = DELAY_1_8_S;
	check_operator_AT.execution_cmd = malloc(strlen(check_operator_cmd) + 1);
	strcpy(check_operator_AT.execution_cmd, check_operator_cmd);

	send_AT_cmd(check_operator_AT);
	free(check_operator_AT.execution_cmd);

	check_operator_AT.response = malloc(sizeof(usart1_rx_buffer) + 1);
	strcpy(check_operator_AT.response, (char*)usart1_rx_buffer);
	clear_uart_rx_buffer();

	char* output_msg = data;
	get_operator_parse_response(check_operator_AT.response, output_msg);

	free(check_operator_AT.response);
	usb_tx_buffer = malloc(strlen(output_msg));
	strcpy((char*)usb_tx_buffer, output_msg);
	CDC_Transmit_FS(usb_tx_buffer, strlen((char*)usb_tx_buffer));
	free(usb_tx_buffer);
	return data;
}

char* set_apn_func(char* data)
{
	set_apn.delay = DELAY_300_MS;
	set_apn.execution_cmd = malloc(strlen(set_apn_cmd) + 1);
	strcpy(set_apn.execution_cmd, set_apn_cmd);

	send_AT_cmd(set_apn);
	free(set_apn.execution_cmd);

	set_apn.response = malloc(sizeof(usart1_rx_buffer) + 1);
	strcpy(set_apn.response, (char*)usart1_rx_buffer);
	clear_uart_rx_buffer();

	usb_tx_buffer = malloc(strlen(set_apn.response));
	strcpy((char*)usb_tx_buffer, set_apn.response);
	free(set_apn.response);
	CDC_Transmit_FS(usb_tx_buffer, strlen((char*)usb_tx_buffer));
	free(usb_tx_buffer);
	return data;
}

char* open_socket(char* data)
{
	open_socket_AT.delay = DELAY_150_S;
	open_socket_AT.execution_cmd = malloc(strlen(open_socket_cmd) + 1);
	strcpy(open_socket_AT.execution_cmd, open_socket_cmd);

	send_AT_cmd(open_socket_AT);
	free(open_socket_AT.execution_cmd);

	open_socket_AT.response = malloc(sizeof(usart1_rx_buffer) + 1);
	strcpy(open_socket_AT.response, (char*)usart1_rx_buffer);
	clear_uart_rx_buffer();

	char* output_msg = data;
	parse_open_socket_response(open_socket_AT.response, output_msg);
	free(open_socket_AT.response);

	usb_tx_buffer = malloc(strlen(output_msg));
	strcpy((char*)usb_tx_buffer, output_msg);
	CDC_Transmit_FS(usb_tx_buffer, strlen((char*)usb_tx_buffer));
	free(usb_tx_buffer);
	return data;
}

char* send_data(char* data)
{
	send_data_AT.delay = DELAY_150_S;
	send_data_AT.execution_cmd = malloc(strlen(send_data_cmd) + 1);
	strcpy(send_data_AT.execution_cmd, send_data_cmd);

	send_AT_cmd(send_data_AT);
	free(send_data_AT.execution_cmd);

	send_data_AT.response = malloc(sizeof(usart1_rx_buffer) + 1);
	strcpy(send_data_AT.response, (char*)usart1_rx_buffer);
	clear_uart_rx_buffer();

	usb_tx_buffer = malloc(strlen(open_socket_AT.response));
	strcpy((char*)usb_tx_buffer, open_socket_AT.response);
	free(send_data_AT.response);
	CDC_Transmit_FS(usb_tx_buffer, strlen((char*)usb_tx_buffer));
	free(usb_tx_buffer);
	return data;
}

void build_pdp_command(PDPSettings* pdp_struct)
{
	pdp_struct->define_PDP_cmd = malloc(PDP_SETTING_CMD_STATIC_LENGTH + MAX_APN_LENGTH);
    sprintf(pdp_struct->define_PDP_cmd, "AT+CGDCONT=%d,%s,%s", pdp_struct->cid, pdp_struct->IP_type, pdp_struct->apn_name);
}


char* query_net_status(char* stat)
{
	query_net_stat.delay = DELAY_300_MS;
	query_net_stat.execution_cmd = malloc(strlen(query_net_stat_cmd) + 1);
	strcpy(query_net_stat.execution_cmd, query_net_stat_cmd);
	send_AT_cmd(query_net_stat);
	free(query_net_stat.execution_cmd);

	query_net_stat.response = malloc(sizeof(usart1_rx_buffer) + 1);
	strcpy(query_net_stat.response, (char*)usart1_rx_buffer);
	clear_uart_rx_buffer();

	NetRegistrationStat net_stat;
	getNetworkRegistrationStatusDescription(query_net_stat.response, &net_stat);
	free(query_net_stat.response);

	usb_tx_buffer = malloc(strlen(net_stat.net_registration_stat_verbose) + 1);
	strcpy((char*)usb_tx_buffer, net_stat.net_registration_stat_verbose);
	free(net_stat.net_registration_stat_verbose);
	CDC_Transmit_FS(usb_tx_buffer, strlen((char*)usb_tx_buffer));
	free(usb_tx_buffer);
	return stat;
}

void modem_info_parser(const char* response, char* imei, size_t imei_size, char* fw_version, size_t fw_size) {
    const char* imei_prefix = "AT+GSN\r\r\n";
    const char* fw_prefix = "AT+GMR\r\r\n";

    // Find the start of the IMEI value
    const char* imei_start = strstr(response, imei_prefix);
    if (imei_start) {
        imei_start += strlen(imei_prefix); // Move pointer past the prefix
        const char* imei_end = strstr(imei_start, "\r\n"); // Find the end of the IMEI value
        if (imei_end) {
            size_t imei_length = imei_end - imei_start;
            if (imei_length < imei_size) {
                strncpy(imei, imei_start, imei_length);
                imei[imei_length] = '\0'; // Null-terminate the string
            }
        }
    }

    // Find the start of the firmware version
    const char* fw_start = strstr(response, fw_prefix);
    if (fw_start) {
        fw_start += strlen(fw_prefix); // Move pointer past the prefix
        const char* fw_end = strstr(fw_start, "\r\n"); // Find the end of the firmware version
        if (fw_end) {
            size_t fw_length = fw_end - fw_start;
            if (fw_length < fw_size) {
                strncpy(fw_version, fw_start, fw_length);
                fw_version[fw_length] = '\0'; // Null-terminate the string
            }
        }
    }
}

void sim_info_parser(const char* response, char* iccid, size_t iccid_size, char* imsi, size_t imsi_size) {
    const char* iccid_prefix = "+QCCID: ";
    const char* imsi_prefix = "AT+CIMI\r\r\n";

    // Find the start of the ICCID value
    const char* iccid_start = strstr(response, iccid_prefix);
    if (iccid_start) {
        iccid_start += strlen(iccid_prefix); // Move pointer past the prefix
        const char* iccid_end = strstr(iccid_start, "\r\n"); // Find the end of the ICCID value
        if (iccid_end) {
            size_t iccid_length = iccid_end - iccid_start;
            if (iccid_length < iccid_size) {
//            	iccid = malloc(iccid_length + 1);
                strncpy(iccid, iccid_start, iccid_length);
                iccid[iccid_length] = '\0'; // Null-terminate the string
            }
        }
    }

    // Find the start of the IMSI value
    const char* imsi_start = strstr(response, imsi_prefix);
    if (imsi_start) {
        imsi_start += strlen(imsi_prefix); // Move pointer past the prefix
        const char* imsi_end = strstr(imsi_start, "\r\n"); // Find the end of the IMSI value
        if (imsi_end) {
            size_t imsi_length = imsi_end - imsi_start;
            if (imsi_length < imsi_size) {
//            	imsi = malloc(imsi_length);
                strncpy(imsi, imsi_start, imsi_length);
                imsi[imsi_length] = '\0'; // Null-terminate the string
            }
        }
    }
}

uint8_t checkSimStatus(const char *response, SIMStatus *sim_stat) {
    // Initialize the status fields
	sim_stat->sim_status = SIM_NOT_READY;

//	memset(status->sim_message, 0, 39 * sizeof(char));

    // Look for "+CPIN: " in the response
    const char *cpinPos = strstr(response, "+CPIN: ");
    if (cpinPos) {
        // Extract the SIM state after "+CPIN: "
        char sim_state[16] = {0};
        sscanf(cpinPos + 7, "%15s", sim_state);

        // Check the state and build the corresponding message
        if (strcmp(sim_state, "READY") == 0) {
        	sim_stat->sim_status = SIM_READY;
        	sim_stat->sim_message = malloc(strlen(sim_ready_msg) + 1);
            strcpy(sim_stat->sim_message, sim_ready_msg);
        } else {
        	sim_stat->sim_message = malloc(strlen(sim_state_msg) + strlen(sim_state) + 2);
            snprintf(sim_stat->sim_message, strlen(sim_state_msg) + strlen(sim_state) + 2, "%s %s", sim_state_msg, sim_state);
        }
    } else {
    	sim_stat->sim_message = malloc(strlen(sim_invalid_state_msg));
    	strcpy(sim_stat->sim_message, sim_invalid_state_msg);
    }

    return sim_stat->sim_status;
}

uint8_t getNetworkRegistrationStatusDescription(const char* response, NetRegistrationStat* net_stat) {
    // Ensure the response is valid and contains "+CEREG: "
    int status = -1;
    const char* prefix = "+CEREG: ";
    const char* ceregPosition = strstr(response, prefix);
    net_stat->net_registration_stat_numeric = status;
    if (!ceregPosition) {
    	char* invalid_stat_unexpected_response = "Invalid response: +CEREG not found.";
    	net_stat->net_registration_stat_verbose = malloc(+ strlen(lte_network_status_intro) + strlen(invalid_stat_unexpected_response) + 1);
        strcpy(net_stat->net_registration_stat_verbose, invalid_stat_unexpected_response);
        return status;
    }

    // Move the pointer to the start of the values
    ceregPosition += strlen(prefix);

    // Extract the second number (registration status)
    if (sscanf(ceregPosition, "%*d,%d", &status) != 1 || status < 0 || status > 5) {
    	char* invalid_stat_parse = "Invalid response: Unable to parse status.\r\n";
    	net_stat->net_registration_stat_verbose = malloc(strlen(invalid_stat_parse) + strlen(lte_network_status_intro) + 1);
        strcpy(net_stat->net_registration_stat_verbose, invalid_stat_parse);
        return status;
    }

    net_stat->net_registration_stat_numeric = status;

    // Store the verbose description based on the status
    switch (status) {
        case NOT_REGISTERED: {
            char* not_registered_stat = "Not registered. MT is not currently searching an operator to register to.\r\n";
            net_stat->net_registration_stat_verbose = malloc(strlen(not_registered_stat) + strlen(lte_network_status_intro) + 1);
            strcpy(net_stat->net_registration_stat_verbose, lte_network_status_intro);
            strcat(net_stat->net_registration_stat_verbose, not_registered_stat);
            break;
        }
        case REGISTERED_HOME_NETWORK: {
            char* registered_home_stat = "Registered, home network.\r\n";
            net_stat->net_registration_stat_verbose = malloc(strlen(registered_home_stat) + strlen(lte_network_status_intro) + 1);
            strcpy(net_stat->net_registration_stat_verbose, lte_network_status_intro);
            strcat(net_stat->net_registration_stat_verbose, registered_home_stat);
            break;
        }
        case SEARCHING_OPERATOR: {
            char* searching_stat = "Not registered, but MT is currently trying to attach or searching an operator to register to.\r\n";
            net_stat->net_registration_stat_verbose = malloc(strlen(searching_stat) + strlen(lte_network_status_intro) + 1);
            strcpy(net_stat->net_registration_stat_verbose, lte_network_status_intro);
            strcat(net_stat->net_registration_stat_verbose, searching_stat);
            break;
        }
        case REGISTRATION_DENIED: {
            char* denied_stat = "Registration denied.\r\n";
            net_stat->net_registration_stat_verbose = malloc(strlen(denied_stat) + strlen(lte_network_status_intro) + 1);
            strcpy(net_stat->net_registration_stat_verbose, lte_network_status_intro);
            strcat(net_stat->net_registration_stat_verbose, denied_stat);
            break;
        }
        case UNKNOWN: {
            char* unknown_stat = "Unknown.\r\n";
            net_stat->net_registration_stat_verbose = malloc(strlen(unknown_stat) + strlen(lte_network_status_intro) + 1);
            strcpy(net_stat->net_registration_stat_verbose, lte_network_status_intro);
            strcat(net_stat->net_registration_stat_verbose, unknown_stat);
            break;
        }
        case REGISTERED_ROAMING: {
            char* roaming_stat = "Registered, roaming.\r\n";
            net_stat->net_registration_stat_verbose = malloc(strlen(roaming_stat) + strlen(lte_network_status_intro) + 1);
            strcpy(net_stat->net_registration_stat_verbose, lte_network_status_intro);
            strcat(net_stat->net_registration_stat_verbose, roaming_stat);
            break;
        }
        default: {
            char* invalid_stat = "Invalid status.\r\n";
            net_stat->net_registration_stat_verbose = malloc(strlen(invalid_stat) + strlen(lte_network_status_intro) + 1);
            strcpy(net_stat->net_registration_stat_verbose, lte_network_status_intro);
            strcat(net_stat->net_registration_stat_verbose, invalid_stat);
            break;
        }
    }
    return status;

}

// Function to parse the response and generate the formatted message
int get_operator_parse_response(const char* response, char* output_message) {
    const char* prefix = "+COPS: ";
    char* start = strstr(response, prefix);
    if (!start) {
        return -1; // Prefix not found
    }

    // Skip to the actual data after "+COPS: "
    start += strlen(prefix);

    // Temporary buffer for the operator name
    char temp_name[14];
    int mode, format, rat_value;

    // Parse the response
    if (sscanf(start, "%d,%d,\"%[^\"]\",%d", &mode, &format, temp_name, &rat_value) != 4) {
        return -1; // Parsing failed
    }

    // Map the RAT value to its string representation
    const char* rat_str;
    switch (rat_value) {
        case GSM:
            rat_str = "GSM";
            break;
        case EMTC:
            rat_str = "EMTC";
            break;
        case NBIOT:
            rat_str = "NBIOT";
            break;
        default:
            rat_str = "Unknown";
            break;
    }

    // Generate the output message
    snprintf(output_message, 70, "Operator name: %s, access technology: %s\r\n", temp_name, rat_str);

    return 0; // Success
}

int parse_open_socket_response(const char* response, char* output_msg) {
    const char* prefix = "+QIOPEN: ";
    char* start = strstr(response, prefix);
    if (!start) {
        return -1; // Prefix not found
    }

    // Move the pointer to the number part
    start += strlen(prefix);

    // Find the last number (after the comma)
    int socket_status = -1;
    if (sscanf(start, "%*d,%d", &socket_status) != 1) {
        return -1; // Parsing failed
    }

    // Generate the appropriate message
    if (socket_status == 0) {
        strcpy(output_msg, "Socket is open successfully\r\n");
    } else {
        sprintf(output_msg, "	Open Socket failed with error code %d\r\n", socket_status);
    }

    return 0; // Success
}





