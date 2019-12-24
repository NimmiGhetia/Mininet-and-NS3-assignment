
from pox.core import core
from pox.lib.packet.ethernet import ethernet, ETHER_BROADCAST
import pox.openflow.libopenflow_01 as of
from pox.lib.addresses import IPAddr,EthAddr
import pox.lib.packet as pkt
from pox.lib.util import dpid_to_str

log = core.getLogger()
switches=0
SwitchMap={}

def launch ():
  """
  Starts the component
  """
  def start_switch (event):
    log.debug("Controlling %s" % (event.connection,))
    #tcpp = event.parsed.find('tcp')
    global switches
    global starttime
    switches+=1
    switchNo=int(dpid_to_str(event.dpid)[-1])
    print ('Conection Up:',switchNo)
    SwitchMap[switchNo]=event
    
    if switchNo == 1:
	# block port 80
	msg = of.ofp_flow_mod()
	msg.match.dl_type = 0x800
	msg.match.nw_dst = IPAddr("10.0.0.4")
	msg.match.nw_proto = 6
	msg.match.tp_dst = 80
	msg.actions.append(of.ofp_action_output(port = of.OFPP_NONE))
	SwitchMap[1].connection.send(msg)
	
	
	print('flow added for switch 1')
       	
	# pushing flow for h0 to h1
	msg = of.ofp_flow_mod()
        msg.match.in_port = 1
        msg.actions.append(of.ofp_action_output(port = 2))
        SwitchMap[1].connection.send(msg)

	# pushing flow for h1 to h0 and outport of switch
        msg = of.ofp_flow_mod()
        msg.match.in_port = 2
        msg.actions.append(of.ofp_action_output(port = 3))
        msg.actions.append(of.ofp_action_output(port = 1))
        SwitchMap[1].connection.send(msg)
	
	# pushing flow for outport of switch s1 to h1
	msg = of.ofp_flow_mod()
        msg.match.in_port = 3
        msg.actions.append(of.ofp_action_output(port = 2))
        SwitchMap[1].connection.send(msg)

    if switchNo == 2:
	# pushing flow for outport of switch s2 to h3
	msg = of.ofp_flow_mod()
        msg.match.in_port = 2
        msg.actions.append(of.ofp_action_output(port = 3))
        SwitchMap[2].connection.send(msg)

	#pushing flow for h3 to outport of switch s2
	msg = of.ofp_flow_mod()
	msg.match.in_port = 3 
	msg.actions.append(of.ofp_action_output(port = 2))
	SwitchMap[2].connection.send(msg)

  core.openflow.addListenerByName("ConnectionUp", start_switch)
