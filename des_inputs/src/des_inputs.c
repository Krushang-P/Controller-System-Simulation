/**************************************************
 * File: des_inputs.c
 * Version: 1.0
 * Author: Krushang Patel
 *
 * Description:
 *  - This file is the client function of the program
 *  - It starts by connecting to the controller
 *  - and send a person structure to it.
 *  - It controls and sets the current state of a person
 *  in the DES application.
 *
 *  Communication: des_inputs <-> des_controller <-> des_display
 *
 **************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <unistd.h>
#include "C:\Users\krush\ide-7.1-workspace\cst8244_assignment_1.ws\des_controller\src\des.h"

/**********************
 * Function declaration
 **********************/
void input_event_prompt(char *input, person_t *person); /* Used to manage state transitions */
void prompt_for_id(person_t *person); /* Used to prompt user for ID */
void prompt_for_weight(person_t *person); /*Used to prompt user for weight */
/*************************************************
 * Function: main
 * Parameters: int, char*
 * Return: int
 *
 * Description: main function of the des_inputs
 * project. des_inputs acts as a client
 * and communicates with the controller
 *
 * the main function will used input_event_prompt
 * to manage and set the current state a person
 * is in while going through the DES system.
 **************************************************/
int main(int argc, char *argv[]) {

	pid_t cpid; /* controller pid storage var */
	int coid; /* return from connect attach  */
	person_t person; /* Person structure defined in des.h */
	ctrl_resp_t controller_response; /* response from the controller, struct defined in des.h */

	/* Check if correct number of command line arguments */
	if (argc != 2) {
		printf("%s\n", errorMessages[IN_ERR_USG]);
		exit(EXIT_FAILURE);
	}

	cpid = atoi(argv[1]); /* get controllers pid from command line arguments */

	/* Connect to controller */
	if ((coid = ConnectAttach(ND_LOCAL_NODE, cpid, 1, _NTO_SIDE_CHANNEL, 0))
			== -1) { /* ON FAIL */
		printf("%s\n", errorMessages[IN_ERR_CONN]);
		exit(EXIT_FAILURE);
	}

	/** DEFAULT get idle going and the ball rolling *//* SEND MESSAGE to controller to confirm communication, once message has been replied, and in idling state*/
	person.state = ST_GRL; /* default IDLE state*//* The Door Entry system is Ready for input once in idling state */
	/*send message early to get controller running idle states */
	if (MsgSend(coid, &person, sizeof(person), &controller_response,
			sizeof(controller_response)) == -1) {
		printf("%s\n", errorMessages[IN_ERR_SND]);
		exit(EXIT_FAILURE);
	}

	while (RUNNING) { /* Infinite Loop */
		char input[5]; /* no valid command is more then 4 chars,but will give more room... NOTE redeclared after every loop */
		printf(
				"Enter the event type (ls = left scan, rs = right scan, ws = weight scale, lo = left open, \n"
						"ro = right open, lc = left closed, rc = right closed, gru = guard right unlock, grl = guard right lock, \n"
						"gll = guard left lock, glu = guard left unlock, exit = exit programs) \n");

		scanf(" %s", input);

		input_event_prompt(input, &person); /*input*/

		/* PHASE II send message to controller */
		if (MsgSend(coid, &person, sizeof(person), &controller_response,
				sizeof(controller_response)) == -1) {
			printf("%s\n", errorMessages[IN_ERR_SND]);
			exit(EXIT_FAILURE);
		}

		/* Check if message is null ( null as in no length) */
		if (sizeof(controller_response) == 0) {
			printf("%s\n", errorMessages[IN_ERR_RSP]);
			exit(EXIT_FAILURE);
		}
		/* Make sure no error messages */
		if (controller_response.statusCode != EOK) { /* DID server generate error?*/
			printf("%s %s\n", errorMessages[ERR_SRVR_MSG],
					controller_response.errMsg);
		}

		if (person.state == ST_EXIT)
			break;
	}
	sleep(5);
	/* Disconnect from server */
	ConnectDetach(coid);
	return EXIT_SUCCESS;

}
/**************************************
 * Function: input_event_prompt
 * Parameters: char *, person_t
 * Return: void
 * Description:
 * 	Function used to transitions through
 * 	program states. Upon user input
 * 	the persons transition/states
 * 	will change.
 **************************************/
void input_event_prompt(char *input, person_t *person) {

	if (strncmp(input, inMessage[IN_LS], strlen(inMessage[IN_LS])) == 0) {
		person->state = ST_LS; /* SET LEFT SCAN STATE */
		prompt_for_id(person);/*prompt*/
	} else if (strncmp(input, inMessage[IN_RS], strlen(inMessage[IN_RS]))
			== 0) {
		person->state = ST_RS;/* SET RIGHT SCAN STATE */
		prompt_for_id(person);/*prompt*/
	} else if (strncmp(input, inMessage[IN_WS], strlen(inMessage[IN_WS]))
			== 0) {
		person->state = ST_WS; /* SET WEIGHT SCALE STATE */
		prompt_for_weight(person);/*prompt*/
	} else if (strncmp(input, inMessage[IN_LO], strlen(inMessage[IN_LO])) == 0)
		person->state = ST_LO; /* SET LEFT OPEN STATE */
	else if (strncmp(input, inMessage[IN_RO], strlen(inMessage[IN_RO])) == 0)
		person->state = ST_RO; /* SET RIGHT OPEN STATE */
	else if (strncmp(input, inMessage[IN_LC], strlen(inMessage[IN_LC])) == 0)
		person->state = ST_LC; /* SET LEFT CLOSE STATE */
	else if (strncmp(input, inMessage[IN_RC], strlen(inMessage[IN_RC])) == 0)
		person->state = ST_RC; /* SET RIGHT CLOSE STATE */
	else if (strncmp(input, inMessage[IN_GRL], strlen(inMessage[IN_GRL])) == 0)
		person->state = ST_GRL; /* SET GUARD RIGHT LOCK */
	else if (strncmp(input, inMessage[IN_GRU], strlen(inMessage[IN_GRU])) == 0)
		person->state = ST_GRU; /* SET GUARD RIGHT UNLOCK */
	else if (strncmp(input, inMessage[IN_GLL], strlen(inMessage[IN_GLL])) == 0)
		person->state = ST_GLL; /* SET GUARD LEFT LOCK */
	else if (strncmp(input, inMessage[IN_GLU], strlen(inMessage[IN_GLU])) == 0)
		person->state = ST_GLU; /* SET GUARD LEFT UNLOCK */
	else if (strncmp(input, inMessage[IN_EXIT], strlen(inMessage[IN_EXIT]))
			== 0)
		person->state = ST_EXIT; /* SET EXIT STATE */

}
/********************
 * Function: prompt_for_id
 * Parameters: person_t
 * Description:
 * Simple function used to prompt user to enter ID
 ********************/
void prompt_for_id(person_t *person) {
	printf("Enter ID:\n"); /* enter ID */
	scanf("%d", &person->id);/* retrieve ID */
}
/********************
 * Function: prompt_for_id
 * Parameters: person_t
 * Description:
 * Simple function used to prompt user to enter weight
 ********************/
void prompt_for_weight(person_t *person) {
	printf("Please Enter your Weight\n");
	scanf("%d", &person->weight);
}

