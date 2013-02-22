/******************************************************************************
 * Copyright (C) 2011 by Jonathan Appavoo, Boston University
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../lib/types.h"
#include "../lib/protocol_client.h"
#include "../lib/protocol_utils.h"

#define STRLEN 81
#define X 'X'
#define O 'O'
#define Q '?'

void initGlobals(char *host, char *port);

struct Globals {
  char host[STRLEN];
  PortType port;
} globals;


typedef struct ClientState  {
  int data;
  char type; // X, O, or ?
  Proto_Client_Handle ph;
} Client;

static int
clientInit(Client *C)
{
  bzero(C, sizeof(Client));
  C->type = Q;
  // initialize the client protocol subsystem
  if (proto_client_init(&(C->ph))<0) {
    fprintf(stderr, "client: main: ERROR initializing proto system\n");
    return -1;
  }
  return 1;
}


static int
update_event_handler(Proto_Session *s)
{
  Client *C = proto_session_get_data(s);

  fprintf(stderr, "%s: called", __func__);
  return 1;
}


int 
startConnection(Client *C, char *host, PortType port, Proto_MT_Handler h)
{
  if (globals.host[0]!=0 && globals.port!=0) {
    if (proto_client_connect(C->ph, host, port)!=0) {
      fprintf(stderr, "failed to connect\n");
      return -1;
    }
    proto_session_set_data(proto_client_event_session(C->ph), C);
#if 0
    if (h != NULL) {
      proto_client_set_event_handler(C->ph, PROTO_MT_EVENT_BASE_UPDATE, 
				     h);
    }
#endif
    return 1;
  }
  return 0;
}


char *
prompt(Client *C, int menu) 
{
  //char *MenuString = malloc(5*sizeof(char));
  //sprintf(MenuString,"\n%c> ",C->type);
  int ret;
  char *c = malloc(sizeof(char) * STRLEN);;

  if (menu) printf("\n%c>",C->type);
  fflush(stdout);
  fgets(c,STRLEN,stdin);
  return c;
}


// FIXME:  this is ugly maybe the speration of the proto_client code and
//         the game code is dumb
int
game_process_reply(Client *C)
{
  Proto_Session *s;

  s = proto_client_rpc_session(C->ph);

  fprintf(stderr, "%s: do something %p\n", __func__, s);

  return 1;
}


int 
doRPCCmd(Client *C, char *c) 
{
  int rc=-1;
/*
  switch (c) {
  case 'h':  
    {
      rc = proto_client_hello(C->ph);
      printf("hello: rc=%x\n", rc);
      if (rc > 0) game_process_reply(C);
    }
    break;
  case 'm':
    scanf("%c", &c);
    rc = proto_client_move(C->ph, c);
    break;
  case 'g':
    rc = proto_client_goodbye(C->ph);
    break;
  default:
    printf("%s: unknown command %c\n", __func__, c);
  }
  */
  // NULL MT OVERRIDE ;-)
  printf("%s: rc=0x%x\n", __func__, rc);
  if (rc == 0xdeadbeef) rc=1;
  return rc;
}

int
doRPC(Client *C)
{
  int rc;
  char *c;

  printf("enter (h|m<c>|g): ");
  fgets(c,STRLEN,stdin);
  rc=doRPCCmd(C,c);

  printf("doRPC: rc=0x%x\n", rc);

  return rc;
}


int 
docmd(Client *C, char *cmd)
{
  int rc = 1, i = 0, j = 0;  
  char *token;
  char *commands[5];
  
  
  token = strtok(cmd, " ");
  commands[i] = (char *) malloc (strlen(token)*sizeof(char));
  strcpy(commands[i], token);
  i++;
  
  while (token != NULL) {
	token = strtok(NULL, ":");
	if (token) {
		commands[i] = (char *) malloc (strlen(token)*sizeof(char));
		strcpy(commands[i], token);
		i++;
	}
  }
	
  /*for (j = 0; j < 5; j++) 
	printf("command[%d] is %s\n", j, commands[j]);
*/  
  
  
  // if i == 1, then other commands. if i > 1, then connect.
  
  if (i == 1) {
  
 	if (streql(cmd,"disconnect\n"))
	{
		rc = doDisconnectCmd(C);
	}
	else if (streql(cmd,"\n"))
	{
		rc = doEnterCmd(C);
	}
	else if (streql(cmd,"where\n"))
	{
		rc = doWhereCmd(C);
	}
	else if (streql(cmd,"quit\n"))
	{
		rc = doQuitCmd(C);
	}
	else if (streql(cmd,"1\n") || streql(cmd,"2\n") || streql(cmd,"3\n")
				|| streql(cmd,"4\n") || streql(cmd,"5\n") || streql(cmd,"6\n")
				|| streql(cmd,"7\n") || streql(cmd,"8\n") || streql(cmd,"9\n")
			)
	{
		rc = doMoveCmd(C,atoi(cmd));
	}
	else
	{
		rc = doDefaultCmd(C);
	}
  }
  
  else {
	// connect 
	rc = doConnectCmd(C,commands[1], commands[2]);
    
  }
  
  return rc;
}

int
doConnectCmd(Client *C, char *host, char *port)
{
    initGlobals(host, port);

    // ok startup our connection to the server
    if ( startConnection(C, globals.host, globals.port, update_event_handler) < 0){
        fprintf(stderr, "ERROR: Not able to connect to %s:%d\n",globals.host, globals.port);
        return -1;
    }else{
        int rc = proto_client_hello(C->ph);
        if (rc == 1){
            fprintf(stdout, "Connected to %s:%d : You are %c\n", globals.host, globals.port,C->type);
        }else if (rc == 0){
            fprintf(stdout, "Connected to %s:%d : You are %c\n", globals.host, globals.port,C->type);
        }
    }
    
    return 1;
}

int
doDisconnectCmd(Client *C)
{
return -1;
}

int
doEnterCmd(Client *C)
{
    
return -1;
}

int
doWhereCmd(Client *C)
{

return -1;
}

int 
doQuitCmd(Client *C)
{
return -1;
}

int
doMoveCmd(Client *C)
{
return -1;
}

int
doDefaultCmd(Client *C)
{
return -1;
}

void *
shell(void *arg)
{
  Client *C = arg;
  char *c;
  int rc;
  int menu=1;

  while (1) {
    if ((c=prompt(C,menu))!=0) rc=docmd(C, c);
    if (rc<0) break;
    if (rc==1) menu=1; else menu=0;
  }

  fprintf(stderr, "terminating\n");
  fflush(stdout);
  return NULL;
}

void 
usage(char *pgm)
{
  fprintf(stderr, "USAGE: %s <port|<<host port> [shell] [gui]>>\n"
           "  port     : rpc port of a game server if this is only argument\n"
           "             specified then host will default to localhost and\n"
	     "             only the graphical user interface will be started\n"
           "  host port: if both host and port are specifed then the game\n"
	     "examples:\n" 
           " %s 12345 : starts client connecting to localhost:12345\n"
	  " %s localhost 12345 : starts client connecting to locaalhost:12345\n",
	  pgm, pgm, pgm, pgm);
 
}

void
initGlobals(char *host, char *port)
{
  bzero(&globals, sizeof(globals));

  strncpy(globals.host, host, STRLEN);
  globals.port = atoi(port);

}

int
streql(char *c1, char *c2)
{
    return strcmp(c1, c2) == 0;
}


int 
main(int argc, char **argv)
{
  Client c;

 // initGlobals(argc, argv);

  if (clientInit(&c) < 0) {
    fprintf(stderr, "ERROR: clientInit failed\n");
    return -1;
  }    

shell(&c);

  return 0;
}

