//===================================================================================================================
//
// file :               device_3.tpp
//
// description :        C++ source code for the Device_3Impl class template methods
//
// project :            TANGO
//
// author(s) :          E.Taurel
//
// Copyright (C) :      2011,2012
//						European Synchrotron Radiation Facility
//                      BP 220, Grenoble 38043
//                      FRANCE
//
// This file is part of Tango.
//
// Tango is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Tango is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License along with Tango.
// If not, see <http://www.gnu.org/licenses/>.
//
// $Revision$
//
//==================================================================================================================

#ifndef _DEVICE_3_TPP
#define _DEVICE_3_TPP

namespace Tango
{

//+---------------------------------------------------------------------------------------------------------------
//
// method :
//		Device_3Impl::set_attribute_config_3_local
//
// description :
//		Set attribute configuration for both AttributeConfig_3 and AttributeConfig_5
//
// args :
//		in :
// 			- new_conf : The new attribute configuration
//			- dummy_arg : Dummy and unnused arg. Just to help template coding
//			- fwd_cb : Set to true if called from fwd att call back
//			- caller_idl : IDL release used by caller
//
//----------------------------------------------------------------------------------------------------------------

template <typename T,typename V>
void Device_3Impl::set_attribute_config_3_local(const T &new_conf,TANGO_UNUSED(const V &dummy_arg),
												bool fwd_cb,int caller_idl)
{
	cout4 << "Entering Device_3Impl::set_attribute_config_3_local" << endl;

//
// Return exception if the device does not have any attribute
//

	long nb_dev_attr = dev_attr->get_attr_nb();
	if (nb_dev_attr == 0)
	{
		Except::throw_exception((const char *)API_AttrNotFound,
				        (const char *)"The device does not have any attribute",
				        (const char *)"Device_3Impl::set_attribute_config_3_local");
	}

//
// Get some event related data
//

	EventSupplier *event_supplier_nd = NULL;
	EventSupplier *event_supplier_zmq = NULL;

	Tango::Util *tg = Tango::Util::instance();

//
// Update attribute config first locally then in database
//

	long nb_attr = new_conf.length();
	long i;

    EventSupplier::AttributeData ad;
    ::memset(&ad,0,sizeof(ad));

	try
	{
		for (i = 0;i < nb_attr;i++)
		{
			Attribute &attr = dev_attr->get_attr_by_name(new_conf[i].name);
			bool old_alarm = attr.is_alarmed().any();

//
// Special case for forwarded attributes
//

			if (attr.is_fwd_att() == true)
			{
				FwdAttribute &fwd_attr = static_cast<FwdAttribute &>(attr);
				if (fwd_cb == true)
					fwd_attr.set_att_config(new_conf[i]);
				else
					fwd_attr.upd_att_config(new_conf[i]);
			}
			else
			{
				attr.set_upd_properties(new_conf[i],device_name);
			}

//
// In case the attribute quality factor was set to ALARM, reset it to VALID
//

			if ((attr.get_quality() == Tango::ATTR_ALARM) &&
				(old_alarm == true) &&
				(attr.is_alarmed().any() == false))
				attr.set_quality(Tango::ATTR_VALID);

//
// Send the event
//

			if (attr.use_notifd_event() == true)
				event_supplier_nd = tg->get_notifd_event_supplier();
			else
				event_supplier_nd = NULL;

			if (attr.use_zmq_event() == true)
				event_supplier_zmq = tg->get_zmq_event_supplier();
			else
				event_supplier_zmq = NULL;

			if ((event_supplier_nd != NULL) || (event_supplier_zmq != NULL))
			{
				string tmp_name(new_conf[i].name);
				const V *tmp_ptr = &(new_conf)[i];

				Tango::AttributeConfig_3 conf3;
				AttributeConfig_3 *tmp_conf_ptr;

				if (get_dev_idl_version() > 4)
				{
					if (caller_idl <= 4)
						::memcpy(&(ad.attr_conf_3),&(tmp_ptr),sizeof(V *));
					else
					{
						if (attr.get_client_lib() == 8)
						{
							attr.AttributeConfig_5_2_AttributeConfig_3(new_conf[i],conf3);
							tmp_conf_ptr = &conf3;
							::memcpy(&(ad.attr_conf_3),&(tmp_conf_ptr),sizeof(V *));

cout << "Pushing AttributeConfig_3 event" << endl;
							if (event_supplier_nd != NULL)
								event_supplier_nd->push_att_conf_events(this,ad,(Tango::DevFailed *)NULL,tmp_name);
							if (event_supplier_zmq != NULL)
								event_supplier_zmq->push_att_conf_events(this,ad,(Tango::DevFailed *)NULL,tmp_name);
							ad.attr_conf_3 = NULL;
						}
cout << "Pushing AttributeConfig_5 event" << endl;
						::memcpy(&(ad.attr_conf_5),&(tmp_ptr),sizeof(V *));
					}
				}
				else
					::memcpy(&(ad.attr_conf_3),&(tmp_ptr),sizeof(V *));

				if (event_supplier_nd != NULL)
					event_supplier_nd->push_att_conf_events(this,ad,(Tango::DevFailed *)NULL,tmp_name);
				if (event_supplier_zmq != NULL)
					event_supplier_zmq->push_att_conf_events(this,ad,(Tango::DevFailed *)NULL,tmp_name);
			}
		}

	}
	catch (Tango::DevFailed &e)
	{

//
// Re build the list of "alarmable" attribute
//

		dev_attr->get_alarm_list().clear();
		for (long j = 0;j < nb_dev_attr;j++)
		{
			Attribute &att = dev_attr->get_attr_by_ind(j);
			if (att.is_alarmed().any() == true)
			{
				if (att.get_writable() != Tango::WRITE)
					dev_attr->get_alarm_list().push_back(j);
			}
		}

//
// Change the exception reason flag
//

		TangoSys_OMemStream o;

		o << e.errors[0].reason;
		if (i != 0)
			o << "\nAll previous attribute(s) have been successfully updated";
		if (i != (nb_attr - 1))
			o << "\nAll remaining attribute(s) have not been updated";
		o << ends;

		string s = o.str();
		e.errors[0].reason = CORBA::string_dup(s.c_str());
		throw;
	}

//
// Re build the list of "alarmable" attribute
//

	dev_attr->get_alarm_list().clear();
	for (i = 0;i < nb_dev_attr;i++)
	{
		Tango::Attribute &attr = dev_attr->get_attr_by_ind(i);
		Tango::AttrWriteType w_type = attr.get_writable();
		if (attr.is_alarmed().any() == true)
		{
			if (w_type != Tango::WRITE)
				dev_attr->get_alarm_list().push_back(i);
		}
	}

//
// Return to caller
//

	cout4 << "Leaving Device_3Impl::set_attribute_config_3_local" << endl;
}


} // End of Tango namespace

#endif /* DEVICE_3_TPP */
