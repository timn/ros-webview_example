
/***************************************************************************
 *  simple_proc.cpp - Simple ROS webview request processor
 *
 *  Created: Mon May 09 11:36:55 2011
 *  Copyright  2006-2011  Tim Niemueller [www.niemueller.de]
 *
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version. A runtime exception applies to
 *  this software (see LICENSE.GPL_WRE file mentioned below for details).
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL_WRE file in the doc directory.
 */

#include <ros/ros.h>
#include <webview_msgs/NavRegistration.h>
#include <webview_msgs/UrlRegistration.h>
#include <webview_msgs/ProcessRequest.h>

#include <cstdio>
#include <csignal>

bool
process_request_cb(webview_msgs::ProcessRequest::Request  &req,
		   webview_msgs::ProcessRequest::Response &resp)
{
  printf("Processing request for %s\n", req.url.c_str());
  resp.code = webview_msgs::ProcessRequest::Response::HTTP_OK;
  resp.wrap_in_page = true;
  resp.title = std::string("Test response: ") + req.url;
  resp.body = std::string("<h1>") + req.url + "</h1>";
  return true;
}


void
handle_signal(int signum)
{
  ros::NodeHandle n;

  ros::ServiceClient clt_ureg =
    n.serviceClient<webview_msgs::UrlRegistration>("/webview/unregister");
  webview_msgs::UrlRegistration usrv;
  usrv.request.url_prefix = "/webview_example";
  usrv.request.service_name = "/webview_example/proc";

  if (! clt_ureg.call(usrv)) {
    printf("Client DE-registration service call failed\n");
    goto shutdown;
  } else if (! usrv.response.success) {
    printf("DE-registration failed: %s\n", usrv.response.error.c_str());
    goto shutdown;
  }

  { // block required for goto
    ros::ServiceClient clt_remnav =
      n.serviceClient<webview_msgs::NavRegistration>("/webview/remove_nav_entry");
    webview_msgs::NavRegistration srv_remnav;
    srv_remnav.request.url = "/webview_example/test";
    srv_remnav.request.name = "Webview Example";

    if (! clt_remnav.call(srv_remnav)) {
      printf("Nav entry removing service call failed\n");
      goto shutdown;
    } else if (! srv_remnav.response.success) {
      printf("Removing nav entry failed: %s\n", srv_remnav.response.error.c_str());
      goto shutdown;
    }
  }

 shutdown:
  ros::shutdown();
}


/** Fawkes application.
 * @param argc argument count
 * @param argv array of arguments
 */
int
main(int argc, char **argv)
{
  ros::init(argc, argv, "rosfawkes", ros::init_options::NoSigintHandler);


  // Note that we have a custom SIGINT handler to allow us to properly
  // unregister from webview! We cannot simply call this when ros::spin()
  // returns because by that time service calls won't work anymore
  signal(SIGINT, handle_signal);

  ros::NodeHandle n;
  ros::ServiceServer srv_proc = n.advertiseService("/webview_example/proc",
						   process_request_cb);

  ros::ServiceClient clt_reg =
    n.serviceClient<webview_msgs::UrlRegistration>("/webview/register");
  webview_msgs::UrlRegistration srv;
  srv.request.url_prefix = "/webview_example";
  srv.request.service_name = "/webview_example/proc";

  if (! clt_reg.call(srv)) {
    printf("Client registration service call failed\n");
    exit(1);
  } else if (! srv.response.success) {
    printf("Registration failed: %s\n", srv.response.error.c_str());
    exit(2);
  }

  ros::ServiceClient clt_addnav =
    n.serviceClient<webview_msgs::NavRegistration>("/webview/add_nav_entry");
  webview_msgs::NavRegistration srv_addnav;
  srv_addnav.request.url = "/webview_example/test";
  srv_addnav.request.name = "Webview Example";

  if (! clt_addnav.call(srv_addnav)) {
    printf("Nav entry adding service call failed\n");
    exit(3);
  } else if (! srv_addnav.response.success) {
    printf("Adding nav entry failed: %s\n", srv_addnav.response.error.c_str());
    exit(4);
  }

  ros::spin();

  return 0;
}
