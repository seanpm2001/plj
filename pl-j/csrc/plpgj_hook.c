/**
 * file name:			plpgj_hook.c
 * description:			PL/pgJ call hook.
 * author:				Laszlo Hornyak
 */
 
#include "postgres.h"
#include "fmgr.h"
#include "plpgj_messages.h"
#include "plpgj_channel.h"
#include "plpgj_message_fns.h"
#include "plpgj_core.h"

#include "utils/portal.h"

//
//proto
//
/**
 * Called for handling an exception from the java backend.
 */
void plpgj_exception_do(error_message);
/**
 * Called for handling sql calls from the java backend.
 */
void plpgj_sql_do(sql_msg msg);
/**
 * Called for handling results from the java backend.
 */
Datum plpgj_result_do(plpgj_result);

void plpgj_log_do(log_message);

//
//impl
//

Datum plpgj_call_hook(PG_FUNCTION_ARGS){
	
	callreq req;
	int message_type;
	
	if(!plpgj_channel_initialized()){
			elog(DEBUG1, "initing channel");
			plpgj_channel_initialize();
	}

	elog(DEBUG1, "entering hook");
	
		
	req = plpgj_create_call(fcinfo);
	plpgj_channel_send((message)req);
	free_message(req);
	
	do{
		void* ansver = NULL;
		elog(DEBUG1, "waiting for ansver");
		ansver = plpgj_channel_receive();
		message_type = plpgj_message_type(ansver);
		switch(message_type){
			case MT_RESULT:
					elog(DEBUG1, "result received.");
					
					break;
			case MT_EXCEPTION:
					elog(DEBUG1, "exception received.");
					plpgj_exception_do((error_message)ansver);
					PG_RETURN_NULL();
					break;
			case MT_SQL:{
					elog(DEBUG1, "sql received.");
					plpgj_sql_do(ansver);
					}
					break;
			case MT_LOG:
					plpgj_log_do((log_message)ansver);
					break;
			default:
				elog(FATAL, "received: unknown message.");
				//TODO perhaps there is a more elegant way to escape from here.
				PG_RETURN_NULL();
		}
		free_message(ansver);

		// here is how to escape from the loop
		elog(DEBUG1,"message handling...");

		if(message_type == MT_RESULT){
			//TODO what now? we should return a value HERE.
			// PG_RETURN...
			
			Datum ret;
			
			plpgj_result res = (plpgj_result)ansver;
			elog(DEBUG1, "number of rows: %d", res->rows);
			elog(DEBUG1, "number of cols: %d", res->cols);
			
			if(res->rows == 1 && res->cols == 1){
				
				
				
			} //else if
			
			//continue here!!
			
			PG_RETURN_NULL();
			//break;
		}
		if(message_type == MT_EXCEPTION){
			PG_RETURN_NULL();
		}
	
	}while( 1 );
	
	PG_RETURN_NULL();
	
}

Datum plpgj_result_do(plpgj_result res){
	return 0;
}

void plpgj_sql_do(sql_msg msg){
	switch(msg->sqltype){
		case SQL_TYPE_STATEMENT:{
			SPI_exec( ((sql_msg_statement)msg)->statement, 0 );
		}
		break;
		case SQL_TYPE_CURSOR_CLOSE:{
			Portal portal;
			sql_msg_cursor_close sql_c_c = (sql_msg_cursor_close)msg;
			
			portal = GetPortalByName(sql_c_c->cursorname);
			if(!PortalIsValid(portal)){
				elog(WARNING,"the portal %s does not exist!", sql_c_c->cursorname);
				//TODO throw back an exception!
			}
			
			PortalDrop(portal, 0);
			
		}
		break;
		case SQL_TYPE_CURSOR_OPEN:{
			Portal portal;
			sql_msg_cursor_open sql_c_o = (sql_msg_cursor_open)msg;
			//TODO: creates constantly bidirectional cursors :(
			portal = CreatePortal(sql_c_o->cursorname, 1, 1);
			
			
		}
		break;
		case SQL_TYPE_FETCH:{
			Portal portal;
			
			/*sql_msg_cursor_fetch sql_c_f = (sql_msg_cursor_fetch)msg;
			
			portal = GetPortalByName(sql_c_f->cursorname);
			if(!PortalIsValid(portal)){
				//TODO send back error.
			}*/
		}
		break;
		default:
			elog(FATAL, "Ooops.");
	}
}

void plpgj_exception_do(error_message msg){
	
	elog(ERROR,"Java side exception occured: \n %s : %s \n ", msg->classname, msg->message );
}

void plpgj_log_do(log_message log){
	int level = DEBUG1;
	if(log == NULL)
		return;
	switch(log -> level){
		case 1:
			level = DEBUG5;
			break;
		case 2:
			level = WARNING;
			break;
		case 3:
			level = INFO;
			break;
		case 5: level = INFO;
	}

	elog(level,"");

}
