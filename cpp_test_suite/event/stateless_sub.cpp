/* 
 * example of a client using the TANGO device api.
 */

#include <tango.h>
#include <assert.h>

#ifndef WIN32
#include <unistd.h>
#endif

#define	coutv	if (verbose == true) cout

using namespace Tango;

bool verbose = false;

class EventCallback : public Tango::CallBack
{
public:
    EventCallback()  { };
    ~EventCallback() { };
    void push_event( Tango::EventData *ed );

	int cb_executed;
	int cb_err;
};

void EventCallback::push_event( Tango::EventData *ed )
{
	coutv << "In callback with error flag = " << ed->err << endl;
    if(ed->err == false)
		cb_executed++;
    else
		cb_err++;
}

int main(int argc, char **argv)
{
	DeviceProxy *device;
	
	if (argc == 1)
	{
		cout << "usage: %s device [-v]" << endl;
		exit(-1);
	}

	string device_name = argv[1];

	if (argc == 3)
	{
		if (strcmp(argv[2],"-v") == 0)
			verbose = true;
	}
	
	try 
	{
		device = new DeviceProxy(device_name);
	}
	catch (CORBA::Exception &e)
	{
		Except::print_exception(e);
		exit(1);
	}

	string att_name ("event_change_tst");
	
	try
	{

//
// Subscribe to event with stateless flag set
//

    	int eventID = 0;
    	const vector< string >  filters;
    	EventCallback *eventCallback = new EventCallback();
		eventCallback->cb_executed = 0;
		eventCallback->cb_err = 0;

        eventID = device->subscribe_event(att_name,Tango::CHANGE_EVENT,eventCallback,filters,true);

//
// Wait for connection and event
//

		Tango_sleep(40);
		
//
// Check error and connection
//

		coutv << "cb err = " << eventCallback->cb_err << endl;
		coutv << "cb executed = " << eventCallback->cb_executed << endl;

		assert (eventCallback->cb_err >= 1);
		assert (eventCallback->cb_executed >= 1);

		cout << "   Stateless connection to event --> OK" << endl;						
	}
	catch (Tango::DevFailed &e)
	{
		Except::print_exception(e);
		exit(-1);
	}
	delete device;
	
	return 0;
}
