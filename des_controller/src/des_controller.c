#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <errno.h>
#include "des.h"

/****************************************
 * Function Declaration
 ****************************************/
void* st_ls(); /*	 LEFT SCAN 			*/
void* st_lo(); /*  LEFT OPEN  		*/
void* st_lc(); /*  LEFT CLOSE 		*/
void* st_rs(); /*  RIGHT SCAN 		*/
void* st_ro(); /*  RIGHT OPEN 		*/
void* st_rc(); /*  RIGHT CLOSE 		*/
void* st_grl(); /*  GUARD RIGHT LOCK 	*/
void* st_gru(); /*  GUARD RIGHT UNLOCK */
void* st_gll(); /*  GUARD LEFT LOCK 	*/
void* st_glu(); /*  GUARD LEFT UNLOCK	*/
void* st_ws(); /*  WEIGHT SCALE 		*/
void* st_start();/*  Start 				*/
void* st_exit(); /*  EXIT 				*/

void reset();

/****************************************
 * GLOBAL VARIABLES
 ****************************************/
ctrl_resp_t controller_response; /* response structure */
person_t person; /* person structure */
int coid, chid, rcvid; /* connection id,Channel,return from MsgReceive message */
FState f_state = st_start; /* Initially start at ready state  TODO pointer to function*/
int lrstate = DEFAULT;
/**********************************
 * Function: main
 * Parameters: int, char*
 * Return: Int
 * Description: main function for the controller of the
 * Door Entry System (DES). The controller
 * communicates with the ./des-inputs project
 * using IPC message passing, to let the controller know
 * in which state the current entry (person) is in.
 * Which then the controller will pass a message to the
 * ./des_display project to display their current state.
 *********************************/
int main(int argc, char *argv[]) {

	pid_t dpid; /* display pid */

	if (argc != 2) {/* Validate correct amount of arguments */
		printf("%s\n", errorMessages[CTRL_ERR_USG]);
		exit(EXIT_FAILURE);
	}

	dpid = atoi(argv[1]); /* Get display pid from cmd line arguments and convert to int */

	/* PHASE I: Create Channel */
	if ((chid = ChannelCreate(0)) == -1) {
		printf("%s\n", errorMessages[CTRL_ERR_CHANNEL_CREATE]);
		exit(EXIT_FAILURE);
	}

	/* PHASE I Connect to display, controller between client (inputs) and display server */
	if ((coid = ConnectAttach(ND_LOCAL_NODE, dpid, 1, _NTO_SIDE_CHANNEL, 0))
			== -1) {
		printf("%s\n", errorMessages[CTRL_ERR_CONN]);
		exit(EXIT_FAILURE);
	}

	printf("%s %d\n", outMessage[OUT_START], getpid()); /* display pid for client */
	reset();/* reset structure to make sure no corrupted data */

	while (RUNNING) {
		/* PHASE II */
		if ((rcvid = MsgReceive(chid, &person, sizeof(person), NULL)) == -1) { /* receive message from client */
			printf("%s\n", errorMessages[CTRL_ERR_RCV]); /* ON FAIL */
			exit(EXIT_FAILURE);
		}

		if (person.state == ST_EXIT) /* if state is exit then move to exit state from any state, avoids adding exit code to every state */
			f_state = (*st_exit)();
		else
			f_state = (FState) (*f_state)(); /* else follow states through function pointers */

		controller_response.statusCode = EOK;/* Good response from server */

		/* Reply to client  if state is not exit*/
		if (person.state != ST_EXIT) /* No need to reply on program termination */
			MsgReply(rcvid, EOK, &controller_response,
					sizeof(controller_response));

		if (person.state == ST_EXIT) /* break infinite loop if Termination with Exit state */
			break;

	}
	ConnectDetach(coid); /* Detach from display */
	ChannelDestroy(chid);/* destroy channel */
	return EXIT_SUCCESS;
}
/**************************************************
 * Function st_start
 * Description: Function pointer used when the
 * person is at the starting state of the door entry
 * system. Function will return the next state based
 * on input.
 *
 * NEXT POSSIBLE STATES:
 * GRL
 *
 * The point of the start function is to receive a first message
 * from the client input to initiate the start of the system
 * into idling state. DEFAULT is GRL. At the end of the des process
 * the idling state can possibily be GLL if the person entered
 * from the right, or be again GRL if use entered from the left
 *
 * All errors are handles before hand
 *************************************************/
void* st_start() { /* START STATE  move to idling state, message has been received from inputs*/
	return st_grl; /* Return the default IDLE state, waiting for a person to scan. */
}
/**************************************************
 * Function st_ls
 * Description: Function pointer used when the
 * person is at the left scan state of the door entry
 * system. Function will return the next state based
 * on input.
 *
 * NEXT POSSIBLE STATES:
 * GLU - Guard left unlock
 *
 *
 * On ERROR returns the current state
 *************************************************/
void* st_ls() { /* LEFT SCAN */
	if (person.state == ST_GLU) {
		if (MsgSend(coid, &person, sizeof(person), &controller_response,
				sizeof(controller_response)) == -1) {
			printf("%s GLU - st_ls()\n", errorMessages[CTRL_ERR_SND]);
			exit(EXIT_FAILURE);
		}
		return st_glu; /* return next state, if scan or input were successful */
	}
	return st_ls;
}
/**************************************************
 * Function st_ls
 * Description: Function pointer used when the
 * person is at the Guard Left Unlock state of the door entry
 * system. Function will return the next state based
 * on input.
 *
 * NEXT POSSIBLE STATES:
 * LO - Left Open
 *
 *
 * On ERROR returns the current state
 *************************************************/
void* st_glu() { /*  GUARD LEFT UNLOCK	*/
	if (person.state == ST_LO) {
		if (MsgSend(coid, &person, sizeof(person), &controller_response,
				sizeof(controller_response)) == -1) {
			printf("%s LO - st_glu()\n", errorMessages[CTRL_ERR_SND]);
			exit(EXIT_FAILURE);
		}
		return st_lo; /* return next state, if successful */
	}
	return st_glu;
}
/**************************************************
 * Function st_lo
 * Description: Function pointer used when the
 * person is at the left open state of the door entry
 * system. Function will return the next state based
 * on input.
 *
 * NEXT POSSIBLE STATES:
 * WS - Weigh Scan
 * LC - Left Close
 *
 * On ERROR returns the current state
 *************************************************/
void* st_lo() { /*  LEFT OPEN  		*/
	if (person.state == ST_WS) { /* IF person entered from the left then they need to weigh in*/
		if (MsgSend(coid, &person, sizeof(person), &controller_response,
				sizeof(controller_response)) == -1) {
			printf("%s WS -st_lo()\n", errorMessages[CTRL_ERR_SND]);
			exit(EXIT_FAILURE);
		}
		return st_ws;
	} else if (person.state == ST_LC) { /* IF person entered from the right, state after LO is LC*/
		if (MsgSend(coid, &person, sizeof(person), &controller_response,
				sizeof(controller_response)) == -1) {
			printf("%s LC - st_lo()\n", errorMessages[CTRL_ERR_SND]);
			exit(EXIT_FAILURE);
		}
		return st_lc;
	}
	return st_lo;
}
/**************************************************
 * Function st_rs
 * Description: Function pointer used when the
 * person is at the right scan state of the door entry
 * system. Function will return the next state based
 * on input.
 *
 * NEXT POSSIBLE STATES:
 * GRU - Guard Right Unlock
 *
 *
 * On ERROR returns the current state
 *************************************************/
void* st_rs() { /* RIGHT SCAN */
	if (person.state == ST_GRU) { /* Person enter by the right door */
		if (MsgSend(coid, &person, sizeof(person), &controller_response,
				sizeof(controller_response)) == -1) {
			printf("%s GRU - st_rs()\n", errorMessages[CTRL_ERR_SND]);
			exit(EXIT_FAILURE);
		}
		return st_gru;
	}
	return st_rs;
}
/**************************************************
 * Function st_gru
 * Description: Function pointer used when the
 * person is at the Guard right unlock state of the door entry
 * system. Function will return the next state based
 * on input.
 *
 * NEXT POSSIBLE STATES:
 * RO - right open
 *
 *
 * On ERROR returns the current state
 *************************************************/
void* st_gru() { /*  GUARD RIGHT UNLOCK */
	if (person.state == ST_RO) { /* let person entire the right door */
		if (MsgSend(coid, &person, sizeof(person), &controller_response,
				sizeof(controller_response)) == -1) {
			printf("%s RO - st_gru()\n", errorMessages[CTRL_ERR_SND]);
			exit(EXIT_FAILURE);
		}
		return st_ro;
	}
	return st_gru;
}
/**************************************************
 * Function st_ls
 * Description: Function pointer used when the
 * person is at the right open state of the door entry
 * system. Function will return the next state based
 * on input.
 *
 * NEXT POSSIBLE STATES:
 * WS - weigh scale
 * RC - right close
 *
 * On ERROR returns the current state
 *************************************************/
void* st_ro() { /*  RIGHT OPEN 		*/
	if (person.state == ST_WS) { /* IF weighing, then person entered from the right*/
		if (MsgSend(coid, &person, sizeof(person), &controller_response,
				sizeof(controller_response)) == -1) {
			printf("%s WS - st_ro()\n", errorMessages[CTRL_ERR_SND]);
			exit(EXIT_FAILURE);
		}
		return st_ws;
	} else if (person.state == ST_RC) { /* if right closed then person entered from the left */
		if (MsgSend(coid, &person, sizeof(person), &controller_response,
				sizeof(controller_response)) == -1) {
			printf("%s RC- st_ro()\n", errorMessages[CTRL_ERR_SND]);
			exit(EXIT_FAILURE);
		}
		return st_rc;
	}
	return st_ro;
}
/**************************************************
 * Function st_ls
 * Description: Function pointer used when the
 * person is at the weigh scale state of the door entry
 * system. Function will return the next state based
 * on input.
 *
 * NEXT POSSIBLE STATES:
 * LC - left close
 * RC - right close
 *
 * On ERROR returns the current state
 *************************************************/
void* st_ws() { /*  WEIGHT SCALE 		*/
	if (person.state == ST_LC) { /* Person entered from the left */
		if (MsgSend(coid, &person, sizeof(person), &controller_response,
				sizeof(controller_response)) == -1) {
			printf("%s LC - st_ws()\n", errorMessages[CTRL_ERR_SND]);
			exit(EXIT_FAILURE);
		}
		return st_lc;
	}
	if (person.state == ST_RC) { /* person entered from the right */
		if (MsgSend(coid, &person, sizeof(person), &controller_response,
				sizeof(controller_response)) == -1) {
			printf("%s RC - st_ws()\n", errorMessages[CTRL_ERR_SND]);
			exit(EXIT_FAILURE);
		}
		return st_rc;
	}
	return st_ws;
}
/**************************************************
 * Function st_lc
 * Description: Function pointer used when the
 * person is at the left close state of the door entry
 * system. Function will return the next state based
 * on input.
 *
 * NEXT POSSIBLE STATES:
 * GLL - Guard left lock
 *
 *
 * On ERROR returns the current state
 *************************************************/
void* st_lc() { /*  LEFT CLOSE 		*/
	if (person.state == ST_GLL) { /* person has left the left doors */
		if (MsgSend(coid, &person, sizeof(person), &controller_response,
				sizeof(controller_response)) == -1) {
			printf("%s GLL - st_lc()\n", errorMessages[CTRL_ERR_SND]);
			exit(EXIT_FAILURE);
		}
		return st_gll;
	}
	return st_lc;
}
/**************************************************
 * Function st_rc
 * Description: Function pointer used when the
 * person is at the right close state of the door entry
 * system. Function will return the next state based
 * on input.
 *
 * NEXT POSSIBLE STATES:
 * GRL - Guard right lock
 *
 *
 * On ERROR returns the current state
 *************************************************/
void* st_rc() { /*  RIGHT CLOSE 		*/
	if (person.state == ST_GRL) { /* person has left the right doors */
		if (MsgSend(coid, &person, sizeof(person), &controller_response,
				sizeof(controller_response)) == -1) {
			printf("%s GRL - st_rc() \n", errorMessages[CTRL_ERR_SND]);
			exit(EXIT_FAILURE);
		}
		return st_grl;
	}
	return st_rc;
}
/**************************************************
 * Function st_grl
 * Description: Function pointer used when the
 * person is at the Guard right lock state of the door entry
 * system. Function will return the next state based
 * on input.
 *
 * ** IDLE STATE **
 * -ROUTES
 * 	 1 - Person entered form the right and exiting from the left : next state GLU
 * 	 2 - Person Entered from the left and has now left the building. Wait for new
 * 	 	 person to enter
 *
 * NEXT POSSIBLE STATES:
 * GLU - Guard Left Unlock
 * RS  - Right scan
 * LS - Left scan
 *
 * On ERROR returns the current state
 *************************************************/
void* st_grl() { /*  GUARD RIGHT LOCK 	*/
	if (lrstate == RIGHT && person.state == ST_GLU) { /* person entered from the right hand side and must exit the left */
		if (MsgSend(coid, &person, sizeof(person), &controller_response,
				sizeof(controller_response)) == -1) {
			printf("%s GLU - st_grl()\n", errorMessages[CTRL_ERR_SND]);
			exit(EXIT_FAILURE);
		}
		lrstate = DEFAULT;
		return st_glu;
	}
	if (lrstate == DEFAULT && person.state == ST_RS) { /* GRl is on idle waiting for person to scan right */
		if (MsgSend(coid, &person, sizeof(person), &controller_response,
				sizeof(controller_response)) == -1) {
			printf("%s RS - st_grl()\n", errorMessages[CTRL_ERR_SND]);
			exit(EXIT_FAILURE);
		}
		lrstate = RIGHT;
		return st_rs;
	}
	if (lrstate == DEFAULT && person.state == ST_LS) {/* GRL is on idle waiting for person to scan left */
		if (MsgSend(coid, &person, sizeof(person), &controller_response,
				sizeof(controller_response)) == -1) {
			printf("%s LS - st_grl()\n", errorMessages[CTRL_ERR_SND]);
			exit(EXIT_FAILURE);
		}
		lrstate = LEFT;
		return st_ls;
	}

	return st_grl;
}
/**************************************************
 * Function st_grl
 * Description: Function pointer used when the
 * person is at the Guard left lock state of the door entry
 * system. Function will return the next state based
 * on input.
 *
 * ** IDLE STATE **
 * -ROUTES
 * 	 1 - Person entered form the left and exiting from the right : next state GRU
 * 	 2 - Person Entered from the right and has now left the building. Wait for new
 * 	 	 person to enter
 *
 * NEXT POSSIBLE STATES:
 * GRU - Guard Left Unlock
 * RS  - Right scan
 * LS  - Left Scan
 *
 * On ERROR returns the current state
 *************************************************/
void* st_gll() { /*  GUARD LEFT LOCK 	*/
	if (lrstate == LEFT && person.state == ST_GRU) { /* person has entered from the left an must now exit from the right */
		if (MsgSend(coid, &person, sizeof(person), &controller_response,
				sizeof(controller_response)) == -1) {
			printf("%s GRU - st_gll()\n", errorMessages[CTRL_ERR_SND]);
			exit(EXIT_FAILURE);
		}
		lrstate = DEFAULT;
		return st_gru;
	} else if (lrstate == DEFAULT
			&& (person.state == ST_RS || person.state == ST_LS)) { /* IDLE state, person entered from the right an has left, wait for next right scan */
		if (MsgSend(coid, &person, sizeof(person), &controller_response,
				sizeof(controller_response)) == -1) {
			printf("%s RS - st_gll()\n", errorMessages[CTRL_ERR_SND]);
			exit(EXIT_FAILURE);
		}
		if (person.state == ST_LS) { /* IDLE state, person entered from the right an has left, wait for next left scan */
			if (MsgSend(coid, &person, sizeof(person), &controller_response,
					sizeof(controller_response)) == -1) {
				printf("%s LS - st_gll()\n", errorMessages[CTRL_ERR_SND]);
				exit(EXIT_FAILURE);
			}
			lrstate = LEFT;
			return st_ls;
		}
		lrstate = RIGHT;
		return st_rs;
	}
	return st_gll;
}
/**************************************************
 * Function st_exit
 * Description: Function pointer used when the
 * person is at the EXIT state of the door entry
 * system. Function will return the next state based
 * on input. EXIT state has no next state, will terminate the
 * door entry system program
 *
 * NEXT POSSIBLE STATES:
 *  NONE: Termination
 *
 * On ERROR returns the current state
 *************************************************/
void* st_exit() { /*  EXIT */

	if (MsgSend(coid, &person, sizeof(person), &controller_response,
			sizeof(controller_response)) == -1) {
		printf("%s EXIT - st_exit()\n", errorMessages[CTRL_ERR_SND]);
		exit(EXIT_FAILURE);
	}
	return st_exit;
}
/**************************************************
 * Function: reset
 * Description: Short function to reset all
 * structure properties, in case of corrupted data
 *************************************************/
void reset() {
	person.id = 0;
	person.weight = 0;
	person.state = ST_START;
}

