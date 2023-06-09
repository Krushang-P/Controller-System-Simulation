#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/neutrino.h>
#include <unistd.h>
#include "C:\Users\krush\ide-7.1-workspace\cst8244_assignment_1.ws\des_controller\src\des.h"

/***********************
 *  Function Declaration
 ***********************/
void display_current_state(person_t *person); /* function to display the current state of a person */
/**********************************
 * Function: main
 * Return: Int
 * Description: main function for the display
 * server of the door entry system. function runs
 * through infinite loop using IPC message passing and
 * communicating with the controller. With the goal
 * to receive person_t struct data. This data is able to tell
 * the display server what is the current state of the person
 * who has entered the building and display it on the console.
 *********************************/
int main(void) {

	person_t person; /* persons message */
	ctrl_resp_t response; /* response message from server */

	int chid, rcvid; /* local variables */

	/***************
	 * PHASE I
	 ***************/
	if ((chid = ChannelCreate(0)) == -1) /* Create Channel */
	{ /* ON FAIL */
		printf("%s\n", errorMessages[DP_ERR_CHANNEL_CREATE]);
		exit(EXIT_FAILURE);
	}

	printf("Display PID = %d\n", getpid()); /* display the Process ID of the server */

	/*****************
	 * PHASE II
	 *****************/

	while (RUNNING) { /* normal behavior of a server infinite loop */

		if ((rcvid = MsgReceive(chid, &person, sizeof(person), NULL)) == -1) { /* receive message from client */
			printf("%s\n", errorMessages[DP_ERR_RCV]); /* ON FAIL */
			exit(EXIT_FAILURE);
		}
		display_current_state(&person); /* display the current state of the person */

		if (person.state != ST_EXIT) {/* No need to reply on system termination */
			if (MsgReply(rcvid, EOK, &response, sizeof(response)) == -1) {
				printf("%s\n", errorMessages[DP_ERR_RPLY]);/* reply EOK back to controller */
			}
		}

		if (response.statusCode != EOK) { /*If status is not EOK return error */
			printf("%s %s\n", errorMessages[ERR_SRVR_MSG], response.errMsg);
		}

		if (person.state == ST_EXIT)
			break;

	}
	ChannelDestroy(chid);
	return EXIT_SUCCESS;

}
/**********************************
 * Function: display_current_state
 * Parameters: person_t
 * Description: This function is used to display
 * the current state of the person trying to enter or
 * has entered the building. Using output messages designed
 * in des.h
 *********************************/
void display_current_state(person_t *person) {

	/* DISPLAY current State messages from outMessage */
	switch (person->state) {
	case ST_LS: /* Left scan*/
	case ST_RS:/*Right scan*/
		printf("%s %d \n", outMessage[OUT_LS_RS], person->id); /* OUT message with person ID card number*/
		break;

	case ST_WS: /* Weigh scale state */
		printf("%s %d \n", outMessage[OUT_WS], person->weight); /* OUT message with person weight*/
		break;
	case ST_LO: /* LEFT OPEN */
		printf("%s\n", outMessage[OUT_LO]);
		break;
	case ST_RO:/* RIGHT OPEN */
		printf("%s\n", outMessage[OUT_RO]);
		break;
	case ST_LC: /* LEFT CLOSE */
		printf("%s\n", outMessage[OUT_LC]);
		break;
	case ST_RC:/* RIGHT CLOSE */
		printf("%s\n", outMessage[OUT_RC]);
		break;
	case ST_GRL: /* GUARD RIGHT LOCK */
		printf("%s\n", outMessage[OUT_GRL]);
		break;
	case ST_GRU: /* GUARD RIGH UNLOCK */
		printf("%s\n", outMessage[OUT_GRU]);
		break;
	case ST_GLL: /* GUARD LEFT LOCK */
		printf("%s\n", outMessage[OUT_GLL]);
		break;
	case ST_GLU:/* GUARD LEFT UNLOCK */
		printf("%s\n", outMessage[OUT_GLU]);
		break;
	case ST_EXIT: /* EXIT */
		printf("%s\n", outMessage[OUT_EXIT]);
		break;
	default: /* ERROR INVALID INPUT */
		printf("ERROR: Invalid input\n");
		break;
	}
}
