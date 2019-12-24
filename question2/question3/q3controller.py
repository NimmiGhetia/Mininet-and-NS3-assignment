
from pox.core import core
from pox.lib.packet.ethernet import ethernet, ETHER_BROADCAST
import pox.openflow.libopenflow_01 as of
from pox.lib.addresses import IPAddr,EthAddr
from pox.lib.packet.arp import arp
import pox.lib.packet as pkt
from pox.lib.packet.icmp import icmp

log = core.getLogger()

# first address is dummy address
mac_addr = []
mac_addr.append(EthAddr("00:00:00:00:00:01"))
mac_addr.append(EthAddr("00:00:00:00:00:01"))
mac_addr.append(EthAddr("00:00:00:00:00:02"))
mac_addr.append(EthAddr("00:00:00:00:00:03"))

ip_addr = []
ip_addr.append(IPAddr("10.0.0.1"))
ip_addr.append(IPAddr("10.0.0.1"))
ip_addr.append(IPAddr("10.0.0.2"))
ip_addr.append(IPAddr("10.0.0.3"))

arpRecieved = False 

class Tutorial (object):
  """
  A Tutorial object is created for each switch that connects.
  A Connection object for that switch is passed to the __init__ function.
  """
  def __init__ (self, connection):
    self.connection = connection
    connection.addListeners(self)
    self.mac_to_port = {}

  def _handle_PacketIn(self,event):
    global arpRecieved
    print('#####################################################')
    packet = event.parsed
    if packet.type == packet.ARP_TYPE:
	print('arp msg')
	arpRecieved = True
        if packet.payload.opcode == arp.REQUEST:
	  print('arp request')
	  # arp request packet from h1 to h2
	  if packet.payload.protosrc == ip_addr[1] and packet.payload.protodst == ip_addr[2]:
	    # sending arp reply to h3 by modifying source as h2 
	    arp_reply = arp()
            arp_reply.hwsrc = mac_addr[3]
            arp_reply.hwdst = packet.payload.hwsrc
            arp_reply.opcode = arp.REPLY
            arp_reply.protosrc = ip_addr[2]
            arp_reply.protodst = packet.payload.protosrc
            ether = ethernet()
            ether.type = ethernet.ARP_TYPE
            ether.dst = packet.src
            ether.src = mac_addr[3]
            ether.payload = arp_reply
            msg = of.ofp_packet_out()
            msg.data = ether.pack()
            msg.actions.append(of.ofp_action_output(port = 1))
            event.connection.send(msg)
	
	  # arp request coming from h3 for h1 
	  if packet.payload.protosrc == ip_addr[3] and packet.payload.protodst == ip_addr[1]:
	    # sending arp reply to h1 by sending the mac address of h3
	    arp_reply = arp()
            arp_reply.hwsrc = mac_addr[1]
            arp_reply.hwdst = packet.payload.hwsrc
            arp_reply.opcode = arp.REPLY
            arp_reply.protosrc = ip_addr[1]
            arp_reply.protodst = packet.payload.protosrc
            ether = ethernet()
            ether.type = ethernet.ARP_TYPE
            ether.dst = packet.src
            ether.src = mac_addr[1]
            ether.payload = arp_reply
            msg = of.ofp_packet_out()
            msg.data = ether.pack()
            msg.actions.append(of.ofp_action_output(port = 3))
            event.connection.send(msg)		
    # handling icmp packets
    if packet.find('icmp'):
      print('icmp packet found')
      icmp_request_packet = packet.payload.payload
      if arpRecieved :    # sometimes hosts are sending icmp packets before sending arp packets, dropping those icmp packets
      	if icmp_request_packet.type == 8:
	  # if icmp packet is for h2 sending it to h3
	  if packet.payload.dstip == ip_addr[2]:
	     packet.payload.dstip = ip_addr[3] 
	     msg = of.ofp_packet_out()
             msg.data = packet
             msg.actions.append(of.ofp_action_output(port = 3))
             event.connection.send(msg)
        
	if icmp_request_packet.type == 0:
	  # seding icmp reply coming from h3 to h1 by changing its src address to h2
	  if packet.payload.srcip == ip_addr[3]:
	     packet.payload.dstip = ip_addr[1]
	     packet.payload.srcip = ip_addr[2]
             msg = of.ofp_packet_out()
             msg.data = packet
             msg.actions.append(of.ofp_action_output(port = 1))
             event.connection.send(msg)


def launch ():
  """
  Starts the component
  """
  def start_switch (event):
    log.debug("Controlling %s" % (event.connection,))
    Tutorial(event.connection)
  core.openflow.addListenerByName("ConnectionUp", start_switch)
