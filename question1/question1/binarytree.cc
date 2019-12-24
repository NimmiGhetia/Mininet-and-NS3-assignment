/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Egemen K. Cetinkaya, Justin P. Rohrer, and Amit Dandekar
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Egemen K. Cetinkaya <ekc@ittc.ku.edu>
 * Author: Justin P. Rohrer    <rohrej@ittc.ku.edu>
 * Author: Amit Dandekar       <dandekar@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center 
 * and
 * Department of Electrical Engineering and Computer Science
 * The University of Kansas
 * Lawrence, KS  USA
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture) and 
 * by NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI)
 *
 * This program reads an upper triangular adjacency matrix (e.g. adjacency_matrix.txt) and
 * node coordinates file (e.g. node_coordinates.txt). The program also set-ups a
 * wired network topology with P2P links according to the adjacency matrix with
 * nx(n-1) CBR traffic flows, in which n is the number of nodes in the adjacency matrix.
 */

// ---------- Header Includes -------------------------------------------------
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <bits/stdc++.h>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/global-route-manager.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/assert.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace std;
using namespace ns3;

// ---------- Prototypes ------------------------------------------------------

vector<vector<double> > readCordinatesFile (std::string node_coordinates_file_name);

NS_LOG_COMPONENT_DEFINE ("GenericTopologyCreation");

int main (int argc, char *argv[]){
  
  double SimTime = 3.00;

  // link parameters
  std::string AppPacketRate ("40Kbps");
  Config::SetDefault  ("ns3::OnOffApplication::PacketSize",StringValue ("1000"));
  Config::SetDefault ("ns3::OnOffApplication::DataRate",  StringValue (AppPacketRate));
  std::string LinkRate ("10Mbps");
  std::string LinkDelay ("2ms");

  srand ( (unsigned)time ( NULL ) );   // generate different seed each time

  // file names
  std::string anim_name ("n-node-ppp.anim.xml");
  std::string node_coordinates_file_name ("scratch/node_coordinates");

  int n_nodes = 3 ;

  CommandLine cmd;
  cmd.AddValue("nodes", "Number of nodes", n_nodes);
  cmd.Parse (argc, argv);
  
  node_coordinates_file_name += "_"+ to_string(n_nodes)+".txt" ;

  vector<vector<double>> coord_array;
  coord_array = readCordinatesFile (node_coordinates_file_name);

  NodeContainer nodes;   // Declare nodes objects
  nodes.Create (n_nodes);

  NS_LOG_INFO ("Create P2P Link Attributes.");

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue (LinkRate));
  p2p.SetChannelAttribute ("Delay", StringValue (LinkDelay));

  NS_LOG_INFO ("Install Internet Stack to Nodes.");

  InternetStackHelper internet;
  internet.Install (NodeContainer::GetGlobal ());

  NS_LOG_INFO ("Assign Addresses to Nodes.");
  int num_Nodes = n_nodes ;
  NetDeviceContainer d_link[num_Nodes-1];
  Ipv4InterfaceContainer iic[num_Nodes-1];
  Ipv4Address address[num_Nodes];
  
  for(int i = 0 ; i < num_Nodes ; i++){
    std::string ip = "10.0.0."+ std::to_string(i+1) ;
    NS_LOG_INFO(ip) ;
    address[i] = ip.c_str() ; 
  }

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.255.255.252");

  NS_LOG_INFO ("Create Links Between Nodes.");

    uint32_t linkCount = 0;
    queue<int> remaining ;
    remaining.push(1) ;
    int createdNodes = 0 ;
    while(true){
        queue<int> newrem ;
        while(!remaining.empty()){
            int curr = remaining.front() ;
            createdNodes++ ;
            remaining.pop() ;

            int leftchild =  curr*2 ;
            // creating link for left child
            if(leftchild <= n_nodes){ 
              newrem.push(leftchild) ;
              NodeContainer n_links = NodeContainer (nodes.Get (curr-1), nodes.Get (leftchild-1));
              NetDeviceContainer n_devs = p2p.Install (n_links);
              ipv4.Assign (n_devs);
              ipv4.NewNetwork ();
              linkCount++;
            }

            int rightchild = curr*2 + 1 ;
            // creating link for right child
            if(rightchild <= n_nodes){
              newrem.push(rightchild) ;  
              NodeContainer n_links = NodeContainer (nodes.Get (curr-1), nodes.Get (rightchild-1));
              NetDeviceContainer n_devs = p2p.Install (n_links);
              ipv4.Assign (n_devs);
              ipv4.NewNetwork ();
              linkCount++;
            }
      }
      if(!newrem.empty())
        swap(remaining,newrem) ;
      else 
        break ;
    }
  
  
  NS_LOG_INFO ("Number of links in the binary tree is: " << linkCount);
  NS_LOG_INFO ("Number of all nodes is: " << nodes.GetN ());

  NS_LOG_INFO ("Initialize Global Routing.");
  

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  

// set position with constant coordinate read from file  
  AnimationInterface anim (anim_name.c_str ());
   anim.SetMaxPktsPerTraceFile(500000);
   for(int i = 0 ; i < num_Nodes ; i++){
     Ptr<Node> n = nodes.Get(i);
     cout << coord_array[i][0] << " " << coord_array[i][1] << endl ;
     anim.SetConstantPosition(n,coord_array[i][0],coord_array[i][1]) ;
   }

  NS_LOG_INFO ("Run Simulation.");

  Simulator::Stop (Seconds (SimTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}


vector<vector<double> > readCordinatesFile (std::string node_coordinates_file_name)
{
  ifstream node_coordinates_file;
  node_coordinates_file.open (node_coordinates_file_name.c_str (), ios::in);
  if (node_coordinates_file.fail ()){
      NS_FATAL_ERROR ("File " << node_coordinates_file_name.c_str () << " not found");
  }
  vector<vector<double> > coord_array;
  int m = 0;
  while (!node_coordinates_file.eof()){
      string line;
      getline (node_coordinates_file, line);

      if (line == "")
        {
          NS_LOG_WARN ("WARNING: Ignoring blank row: " << m);
          break;
        }

      istringstream iss (line);
      double coordinate;
      vector<double> row;
      int n = 0;
      while (iss >> coordinate){
          row.push_back (coordinate);
          n++;
      }

      if (n != 2){
        NS_LOG_ERROR ("ERROR: Number of elements at line#" << m << " is "  << n << " which is not equal to 2 for node coordinates file");
        exit (1);
      }
      else{
        coord_array.push_back (row);
      }
      m++;
    }

  node_coordinates_file.close ();
  return coord_array;
}


